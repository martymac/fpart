#!/bin/sh

# Copyright (c) 2014 Ganael LAPLANCHE <ganael.laplanche@martymac.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

# This script is a simple wrapper showing how fpart can be used to migrate data.
# It uses fpart and rsync to spawn multiple rsync instances to migrate data
# from src_dir/ to dst_dir/.
# This migration is incremental and will need a final rsync pass
# (rsync -av -delete src_dir/ dst_dir/) to remove extra files from dst_dir/.

########## Helper functions

usage () {
    echo "$0 <src_dir> <dst_dir>"
}

end_die () {
    [ -n "$1" ] && echo -e "$1" 1>&2
    exit 1
}

is_null () {
    echo "$1" | grep -qE "^[[:space:]]*$"
}

# Handle ^C : stop queue processing by setting the "stop" flag
trap_queue () {
    job_queue_stop
    end_die "\nKilled."
}
trap 'trap_queue' 2

########## Needed for config set-up

# Check for args presence
if is_null "$1" || is_null "$2"
then
    usage
    end_die
fi
SRC_PATH="$1"
DST_PATH="$2"

[ ! -d "${SRC_PATH}" ] && \
    end_die "Directory ${SRC_PATH} does not exist (or is not a directory)"

[ ! -d "${DST_PATH}" ] && \
    end_die "Directory ${DST_PATH} does not exist (or is not a directory)"

# Read the job name
while read -p "Enter a job name: " FPART_JOBNAME && \
    echo "${FPART_JOBNAME}" | grep -vqE '^[a-zA-Z0-9]+$'
do
    :
done

########## Config

# Queue manager configuration. This queue remains local, even when using ssh.
JOBS_MAX=4                                              # How many jobs to run
#JOBS_MAX=$(($(sysctl -n hw.ncpu) - 1))
JOBS_QUEUEDIR="/var/tmp/fpart/queue/${FPART_JOBNAME}"   # Queue dir.
JOBS_WORKDIR="/var/tmp/fpart/work/${FPART_JOBNAME}"     # Current jobs' dir.

# Paths for executables
FPART_BIN="/usr/local/bin/fpart"
RSYNC_BIN="/usr/local/bin/rsync"
SSH_BIN="/usr/bin/ssh"

# Fpart paths. Those ones must be shared amongst all nodes when using ssh.
FPART_LOGDIR="/mnt/nfsshared/fpart/log/${FPART_JOBNAME}"
FPART_OUTPARTDIR="/mnt/nfsshared/fpart/partitions/${FPART_JOBNAME}"

# Fpart options
FPART_MAXPARTFILES="20000"
FPART_MAXPARTSIZE="$((10 * 1024 * 1024 * 1024))" # 10 GB
FPART_OPTIONS="-x '.zfs' -x '.snapshot*'"

# Fpart hooks
FPART_COMMAND="/bin/sh -c '${RSYNC_BIN} -av --numeric-ids \
        --files-from=\\\"\${FPART_PARTFILENAME}\\\" \
        \\\"${SRC_PATH}/\\\" \
        \\\"${DST_PATH}/\\\"' \
        1>\"${FPART_LOGDIR}/$$-\${FPART_PARTNUMBER}.stdout\" \
        2>\"${FPART_LOGDIR}/$$-\${FPART_PARTNUMBER}.stderr\""
FPART_POSTHOOK="echo \"${FPART_COMMAND}\" > \
        \"${JOBS_QUEUEDIR}/\${FPART_PARTNUMBER}\" && \
        touch \"${JOBS_QUEUEDIR}/\${FPART_PARTNUMBER}\" && \
        echo \"=> [FPART] Partition \${FPART_PARTNUMBER} written\"" # XXX [1]

# [1] Workaround for st_mtim.tv_nsec not being initialized at file creation
# on FreeBSD/UFS2 which prevents queue from being processed in the right
# order. This hack forces the update of st_mtim.tv_nsec by using touch just
# after file creation.

# Mail
MAIL_ADDR="storage@mydomain.mytld"

# SSH hosts (comment out to execute jobs locally)
#SSH_HOSTS="login@host1 login@host2 login@host3"

########## Jobs-related functions

# Initialize job queue and work directories
init_job_queue () {
    mkdir -p "${JOBS_QUEUEDIR}" 2>/dev/null || \
        end_die "Cannot create job queue directory ${JOBS_QUEUEDIR}"
    rm -f "${JOBS_QUEUEDIR}"/*
    mkdir -p "${JOBS_WORKDIR}" 2>/dev/null || \
        end_die "Cannot create job work directory ${JOBS_WORKDIR}"
    rm -f "${JOBS_WORKDIR}"/*
}

# Set the "done" flag within job queue
job_queue_done () {
    sleep 1 # Ensure the file gets created within
            # the next second of last file's mtime.
            # Necessary for filesystems that don't
            # get below the second for mtime precision (msdosfs).
    touch "${JOBS_QUEUEDIR}/done"
}

# Set the "stop" flag within job queue
job_queue_stop () {
    touch "${JOBS_QUEUEDIR}/stop"
}

# Get next job name relative to ${JOBS_WORKDIR}/
# Returns empty string if no job is available
dequeue_job () {
    local _NEXT=""
    if [ -f "${JOBS_QUEUEDIR}/stop" ]
    then
        echo "stop"
    else
        _NEXT=$(cd "${JOBS_QUEUEDIR}" && ls -rt1 | head -n 1)
        if [ -n "${_NEXT}" ]
        then
            mv "${JOBS_QUEUEDIR}/${_NEXT}" "${JOBS_WORKDIR}" || \
                end_die "Cannot dequeue next job"
            echo "${_NEXT}"
        fi
    fi
}

# Push a PID to the list of currently-running-PIDs
push_pid_queue () {
    if [ -n "$1" ]
    then
        JOBS_QUEUE="${JOBS_QUEUE} $1"
        JOBS_NUM="$((${JOBS_NUM} + 1))"
    fi
}

# Rebuild the currently-running-PIDs' list by examining each process' state
refresh_pid_queue () {
    local _JOBS_QUEUE=""
    local _JOBS_NUM=0
    for _PID in ${JOBS_QUEUE}
    do
        if ps ${_PID} 1>/dev/null 2>&1
        then
            _JOBS_QUEUE="${_JOBS_QUEUE} ${_PID}"
            _JOBS_NUM="$((${_JOBS_NUM} + 1))"
        fi
    done
    JOBS_QUEUE=${_JOBS_QUEUE}
    JOBS_NUM=${_JOBS_NUM}
}

# Simple round-robin for SSH_HOSTS
rotate_ssh_hosts () {
    if [ -n "${SSH_HOSTS}" ]
    then
        SSH_HOSTS="$(echo ${SSH_HOSTS} | cut -d ' ' -f 2-) $(echo ${SSH_HOSTS} | cut -d ' ' -f 1)"
    fi
}

# Pick-up next SSH host
next_ssh_host () {
    if [ -n "${SSH_HOSTS}" ]
    then
        echo "${SSH_HOSTS}" | cut -d ' ' -f 1
    fi
}

# Main jobs loop : pick up jobs within the queue directory and start them
jobs_loop () {
    local _NEXT=""
    while [ "${_NEXT}" != "done" ] && [ "${_NEXT}" != "stop" ]
    do
        local _PID=""
        if [ ${JOBS_NUM} -lt ${JOBS_MAX} ]
        then
            _NEXT="$(dequeue_job)"
            if [ -n "${_NEXT}" ] && [ "${_NEXT}" != "done" ] && [ "${_NEXT}" != "stop" ]
            then
                echo "=> [QMGR] Starting job ${JOBS_WORKDIR}/${_NEXT}"
                if [ -z "${SSH_HOSTS}" ]
                then
                    /bin/sh "${JOBS_WORKDIR}/${_NEXT}" &
                else
                    "${SSH_BIN}" "$(next_ssh_host)" '/bin/sh -s' \
                        < "${JOBS_WORKDIR}/${_NEXT}" &
                    rotate_ssh_hosts
                fi
                push_pid_queue $!
            fi
        fi
        refresh_pid_queue
        sleep 0.2
    done

    if [ "${_NEXT}" == "done" ]
    then
        echo "=> [QMGR] Done submitting jobs. Waiting for them to finish."
    else
        echo "=> [QMGR] Stopped. Waiting for jobs to finish."
    fi
    wait
    echo "=> [QMGR] No more job running."
}

########## Program start

FPART_OUTPARTTEMPL="${FPART_OUTPARTDIR}/part-$$"

JOBS_NUM=0      # Current num of concurrent processes
JOBS_QUEUE=""   # Jobs PID queue

# Checks for binaries
if [ ! -x "${FPART_BIN}" ] || [ ! -x "${RSYNC_BIN}" ] || [ ! -x "${SSH_BIN}" ]
then
    end_die "External tools are missing, check your configuration"
fi

# Create fpart directories
mkdir -p "${FPART_OUTPARTDIR}" 2>/dev/null || \
    end_die "Cannot create output directory: ${FPART_OUTPARTDIR}"
mkdir -p "${FPART_LOGDIR}" 2>/dev/null || \
    end_die "Cannot create log directory: ${FPART_LOGDIR}"

# Initialize jobs queue and start jobs_loop
init_job_queue
jobs_loop&

# Let's rock !
echo "======> [$$] Syncing ${SRC_PATH} => ${DST_PATH}" | \
    tee -a ${FPART_LOGDIR}/fpart.log
echo "===> Start time : $(date)" | \
    tee -a ${FPART_LOGDIR}/fpart.log
echo "===> Starting fpart..." | \
    tee -a ${FPART_LOGDIR}/fpart.log
echo "===> (parts dir : ${FPART_OUTPARTDIR})" | \
    tee -a ${FPART_LOGDIR}/fpart.log
echo "===> (log dir : ${FPART_LOGDIR})" | \
    tee -a ${FPART_LOGDIR}/fpart.log

# Start fpart from src_dir/ directory and produce jobs within ${JOBS_QUEUEDIR}/
cd "${SRC_PATH}" && \
    time ${FPART_BIN} -f "${FPART_MAXPARTFILES}" -s "${FPART_MAXPARTSIZE}" \
        -o "${FPART_OUTPARTTEMPL}" ${FPART_OPTIONS} -Z -L \
        -W "${FPART_POSTHOOK}" . 2>&1 | tee -a ${FPART_LOGDIR}/fpart.log

# Tell jobs_loop that crawling has finished
job_queue_done

# Wait for jobs_loop to terminate
echo "===> Jobs submitted, waiting..."
wait

# Examine results and send e-mail if requested
RET=$(find ${FPART_LOGDIR}/ -name "$$-*.stderr" ! -size 0)
if [ -n "${MAIL_ADDR}" ]
then
    (echo -e "Return code: $( ([ -z "${RET}" ] && echo '0') || echo '1')" ; \
    [ -n "${RET}" ] && \
        echo -e "Rsync errors detected, see logs:\n${RET}") | \
    mail -s "Fpart job ${FPART_JOBNAME}" "${MAIL_ADDR}"
fi

echo "<=== End time : $(date)" | \
    tee -a "${FPART_LOGDIR}/fpart.log"

[ -n "${RET}" ] && exit 1
exit 0
