#!/bin/bash

run_dual()
{
    benchs="$1"
    nps="$2"
    classes="$3"
    ben_dir="$4"
    bentype=$5

    for class in $classes; do
        for np in $nps; do
            for obench in $benchs; do
                for ibench in $benchs; do
                    echo "running obench = $obench, ibench = $ibench" > \
                        /dev/stderr
                    do_run_dual_$bentype $obench $ibench $class  $np "$ben_dir"
                done
            done
        done
    done
}

run_togather()
{
    benchs="$1"
    class=$2
    nprocs=$3
    bendir="$4"
    runlog="$5"
    bentype=$6

    make_ben_$bentype "$benchs" "$classes" "$nprocs" "$bendir"
    runseq=$(../common/rand_perm $benchs)
    echo "Run seq: $runseq" >> $runlog 
    for bench in $runseq; do
        do_run_free_$bentype $bench $class $nprocs "$bendir" &
    done
    wait
}

run_cross()
{
    benchs="$1"
    class=$2
    nprocs=$3
    bendir="$4"
    bentype=$5
    memaff="0 1"
    cpuaff="0 1"
    # Obench is always on cpu node 0
    ocpu=0
    for obench in $benchs; do
        for ibench in $benchs; do
            for omem in $mem_aff; do
                for imem in $mem_aff; do
                    for icpu in $cpu_aff; do
                        do_run_cross_$bentype $obench $ibench $class \
                            $nprocs $bencdir $ocpu $icpu $omem $imem
                    done
                done
            done
        done
    done
}

run_single()
{
    benchs="$1"
    class=$2
    nprocs=$3
    bendir="$4"
    runlog="$5"
    bentype=$6
    
    for bench in $benchs; do
        echo "running bench=$bench class=$class nprocs=$nprocs" > /dev/stderr
        do_run_free_$bentype $bench $class $nprocs "$bendir"
    done
}

run_solo()
{
    bench=$1
    class=$2
    nprocs=$3
    bendir="$4"
    runlog="$5"
    bentype=$6

    echo "running $bench $class $nprocs" > /dev/stderr
    do_run_free_$bentype $bench $class $nprocs "$bendir"
}

run_nice()
{
    benchs="$1"
    class=$2
    nprocs=$3
    bendir="$4"
    bentype=$5
    nicelist="$6"

    for bench in $benchs; do
        for ni in $nicelist; do
            do_run_free_nice_$bentype $bench $class $nprocs "$bendir" $ni &
        done
    done
    wait
}

run_perf()
{
    benchs="$1"
    class=$2
    nprocs=$3
    bendir="$4"
    bentype=$5

    for bench in $benchs; do
        do_run_bind_socket_perf_$bentype $bench $class $nprocs "$bendir"
    done
}
