#!/bin/bash

show_run_info()
{
    benchs=$1
    classes=$2
    nprocs=$3
    cmd=$4
    echo "Configure"
    echo "benchs = $benchs"
    echo "classes = $classes"
    echo "nprocs = $nprocs"
    echo "running cmd = $cmd"
}

after_run()
{
    bentype=$1
    bendir="$2"
    extra=$3
    runlog="$4"
    benchmark=$5
    email=$6
    res=${7:="$(date +%F-%H-%M-%S)-${bentype}-${extra}.tar.bz2"}

    pushd "${bendir}/output"
    tar -cjf "$res" *.txt
    cat "$runlog" | mail $email -s "$benchmark finished" -a "$res"
    rm *.txt
    popd
}

count_running()
{
    pname=$1
    pgrep -c $pname
}

at_least_one_running()
{
    bench1=$1
    bench2=$2
    
    if [ $(count_runnig $bench1) -gt '0' ] || [ $(count_runnig $bench2) -gt '0' ]; then
        return 0
    fi
    return 1
}

# is bench1 need restart
need_restart()
{
    bench1=$1
    bench2=$2

    #echo "$bench1 $(count_running $bench1)" > /dev/stderr
    #echo "$bench2 $(count_running $bench2)" > /dev/stderr
    if [ $bench1 != $bench2 ] && [ $(count_running $bench2) -eq 1 ] && [ $(count_running $bench1) -lt 1 ]; then
        #echo "Need" > /dev/stderr
        return 0
    fi
    #echo "No need" > /dev/stderr
    return 1
}
