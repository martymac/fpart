#!/bin/sh

MAX_JOBS=$(($(sysctl -n hw.ncpu) - 1))
SEM_JOBNAME="test"
FPART_QUEUEDIR="/mnt/nfsshared/fpart/queue/${SEM_JOBNAME}"
FPART_WORKDIR="/mnt/nfsshared/fpart/work/${SEM_JOBNAME}"

JOBS_NUM=0      # Current num of concurrent processes
JOBS_QUEUE=""   # Jobs PID queue

end_die () {
    [ -n "$1" ] && echo -e "$1" 1>&2
    exit 1
}

# Handle ^C : stop queue processing by setting the "done" flag
trap_queue () {
    job_queue_done
    end_die "\nKilled. (child processes still executing)."
}
trap 'trap_queue' 2

# Initialize job queue and work directories
init_job_queue () {
    mkdir -p "${FPART_QUEUEDIR}" 2>/dev/null || \
        end_die "Cannot create job queue directory ${FPART_QUEUEDIR}"
    rm -f "${FPART_QUEUEDIR}"/*
    mkdir -p "${FPART_WORKDIR}" 2>/dev/null || \
        end_die "Cannot create job work directory ${FPART_WORKDIR}"
    rm -f "${FPART_WORKDIR}"/*
}

# Set the "done" flag within job queue
job_queue_done () {
    sleep 1
    touch "${FPART_QUEUEDIR}/done"
}

# Get next job name relative to ${FPART_WORKDIR}/
# Returns empty string if no job is available
dequeue_job () {
    local _NEXT=""
    _NEXT=$(cd "${FPART_QUEUEDIR}" && ls -rt1 | head -n 1)
    if [ -n "${_NEXT}" ]
    then
        mv "${FPART_QUEUEDIR}/${_NEXT}" "${FPART_WORKDIR}" || \
            end_die "Cannot dequeue next job"
    fi
    echo "${_NEXT}"
}

push_pid_queue () {
    if [ -n "$1" ]
    then
        JOBS_QUEUE="${JOBS_QUEUE} $1"
        JOBS_NUM="$((${JOBS_NUM} + 1))"
    fi
}

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

jobs_loop () {
    local _NEXT=""
    while [ "${_NEXT}" != "done" ]
    do
        local _PID=""
        if [ ${JOBS_NUM} -lt ${MAX_JOBS} ]
        then
            _NEXT="$(dequeue_job)"
            if [ -n "${_NEXT}" ] && [ "${_NEXT}" != "done" ]
            then
                sh "${FPART_WORKDIR}/${_NEXT}" &
                push_pid_queue $!
            fi
        fi
        refresh_pid_queue
        sleep 0.3
    done
    wait
}

# Initialize jobs queue and start jobs_loop
init_job_queue
jobs_loop&

# Execute fpart to produce jobs within ${FPART_QUEUEDIR}/
_JOBNAME=1 ; echo "echo ${_JOBNAME} ; sleep 20" > "${FPART_QUEUEDIR}/${_JOBNAME}"
_JOBNAME=2 ; echo "echo ${_JOBNAME} ; sleep 20" > "${FPART_QUEUEDIR}/${_JOBNAME}"

# Indicate to jobs_loop that crawling has finished
job_queue_done

# Wait for jobs_loop to terminate
wait

exit 0
