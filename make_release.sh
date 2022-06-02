#!/bin/sh

# Generates a release tarball

_release=$(git tag | tail -n 1)
_target_dir="/tmp"

# Print a message to stderr and exit with error code 1
end_die () {
    [ -n "$1" ] && echo "$1" 1>&2
    exit 1
}

cd $(dirname "${0}")/ || \
  end_die "Cannot change directory"

echo "Checking out tag ${_release}..."
{ git checkout "${_release}" && \
  [ "$(git status --porcelain)" = "" ] ;} || \
  end_die "Current branch not clean, cannot proceed"

echo "Copying sources to ${_target_dir}/${_release}..."
{ mkdir -p "${_target_dir}/" && \
  cp -Rf ./ "${_target_dir}/${_release}" ;} || \
  end_die "Error copying sources"

echo "Running autoreconf..."
echo "=="
(cd "${_target_dir}/${_release}" && \
  autoreconf -fi && echo "==") || \
  end_die "Error running autoreconf"

echo "Generating tarball..."
{ tar c -C "${_target_dir}" -zf "${_target_dir}/${_release}.tar.gz" \
    --exclude ".git*" --exclude "autom4te.cache" "${_release}/" ;} || \
  end_die "Error generating file: ${_target_dir}/${_release}.tar.gz"

echo "Created file: ${_target_dir}/${_release}.tar.gz"

echo "Signing release..."
gpg --detach-sign --armor ${_target_dir}/${_release}.tar.gz || \
  end_die "Could not sign tarball"

echo "Created signature: ${_target_dir}/${_release}.tar.gz.asc"

echo "Cleaning up temporary directory..."
rm -rf "${_target_dir}/${_release}" || \
  end_die "Error cleaning up: ${_target_dir}/${_release}"

echo "done."
exit 0
