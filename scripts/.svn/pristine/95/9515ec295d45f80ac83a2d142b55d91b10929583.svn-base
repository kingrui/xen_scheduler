#!/bin/bash

do_run_free_mpi()
{
    bench=$1
    class=$2
    nprocs=$3
    ben_dir="$4"
    nice=$5

    pname=$(./get_pname $bench $class $nprocs)
    out_file=$(./out_name mpi $bench $class $nprocs free)

    mpirun -np $nprocs \
        "${ben_dir}/bin/${pname}" &> "${ben_dir}/output/${out_file}"
}


do_run_free_nice_mpi()
{
    bench=$1
    class=$2
    nprocs=$3
    ben_dir="$4"
    nice=$5

    pname=$(./get_pname $bench $class $nprocs)
    out_file=$(./out_name mpi $bench $class $nprocs free)

    nice -n $nice mpirun -np $nprocs \
        "${ben_dir}/bin/${pname}" &> "${ben_dir}/output/${out_file}"
}

do_run_bind_socket_mpi()
{
    bench=$1
    class=$2
    nprocs=$3
    ben_dir="$4"
    pname=$(./get_pname $bench $class $nprocs)
    out_file=$(./out_name mpi $bench $class $nprocs bind-socket)

    mpirun -np $nprocs -bind-to-socket -report-bindings \
        "${ben_dir}/bin/${pname}" &> "${ben_dir}/output/${out_file}"
}

do_run_bind_socket_perf_mpi()
{
    bench=$1
    class=$2
    nprocs=$3
    bendir="$4"
    pname=$(get_pname $bench $class $nprocs)
    outfile=$(../common/out_name mpi $bench $class $nprocs bind-socket)
    perfout=$(../common/out_name mpi $bench $class $nprocs perf)

    (perf stat mpirun -np $nprocs -bind-to-socket -report-bindings \
        "${bendir}/bin/${pname}" > "${bendir}/output/${outfile}") 2> \
        "${bendir}/output/${perfout}"
}

do_run_dual_bind_socket_mpi()
{
    obench=$1
    ibench=$2
    class=$3
    nprocs=$4
    ben_dir="$5"
    ob_pname=$(./get_pname $obench $class $nprocs)
    ib_pname=$(./get_pname $ibench $class $nprocs)
    ob_path="${ben_dir}/bin/${ob_pname}"
    ib_path="${ben_dir}/bin/${ib_pname}" 
    counter=0
    ob_out=$(./out_name mpi ${obench}-${ibench} $class $nprocs "bind-socket")
    ib_out=$(./out_name mpi ${ibench}-${obench} $class $nprocs \
        "bind-socket-${counter}")
    
    mpirun -np $nprocs -bind-to-socket -report-bindings -slot-list 1:0-3 \
        "$ib_path" &> "${ben_dir}/output/$ib_out" & 
    mpirun -np $nprocs -bind-to-socket -report-bindings -slot-list 1:0-3 \
        "$ob_path" &> "${ben_dir}/output/$ob_out" & 
    sleep 1
    while [ $(count_running $ob_pname) -ge 1 ]; do
        if need_restart $ib_pname $ob_pname; then
            counter=$((counter + 1))
            echo "Restarting $ib_pname" > /dev/stderr
            mpirun -np $nprocs -bind-to-socket -report-bindings -slot-list \
                1:0-3 "$ib_path" &> "${ben_dir}/output/$(./out_name \
                mpi ${ibench}-${obench} $class $nprocs bind-socket-$counter)" &
        fi
        sleep 1
    done
    echo "loop ends" > /dev/stderr
    # wait benchmark
    echo "waiting" > /dev/stderr
    wait
}

do_run_cross_mpi()
{
    obench=$1
    ibench=$2
    class=$3
    nprocs=$4
    ben_dir="$5"
    ocpu=$6
    icpu=$7
    omem=$8
    imem=$9
    ob_pname=$(./get_pname $obench $class $nprocs)
    ib_pname=$(./get_pname $ibench $class $nprocs)
    ob_path="${ben_dir}/bin/${ob_pname}"
    ib_path="${ben_dir}/bin/${ib_pname}"
    counter=0
    # mem-this-that cpu-this-that
    ob_out=$(./out_name mpi ${obench}-${ibench} $class $nprocs \
        mem${omem}-${imem}-cpu${ocpu}-${icpu})
    ib_out=$(./out_name mpi ${ibench}-${obench} $class $nprocs \
        mem${imem}-${omem}-cpu${icpu}-${ocpu}-${counter})
    
    mpirun -np $nprocs numactl --membind=$imem \
        --physcpubind=$((4 + icpu)),$((6 + icpu)) \
        "$ib_path" &> "${ben_dir}/output/${ib_out}" & 
    mpirun -np $nprocs numactl --membind=$omem \
        --physcpubind=$((0 + ocpu)),$((2 + ocpu)) \
        "$ob_path" &> "${ben_dir}/output/${ob_out}" & 

    sleep 1
    while [ $(count_running $ob_pname) -ge 1 ]; do
        if need_restart $ib_pname $ob_pname; then
            counter=$((counter + 1))
            echo "Restarting $ib_pname" > /dev/stderr
            mpirun -np $nprocs numactl --membind=$imem \
                --physcpubind=$((4 + icpu)),$((6 + icpu)) \
                "$ib_path" &> "${ben_dir}/output/$(./out_name \
                mpi ${ibench}-${obench} $class $nprocs \
                mem${imem}-${omem}-cpu${icpu}-${ocpu}-${counter})" &
        fi
        sleep 1
    done
    #echo "loop ends" > /dev/stderr
    # wait benchmark
    echo "waiting" > /dev/stderr
    wait
}



make_ben_mpi()
{
    benchs="$1"
    classes="$2"
    nprocs="$3"
    benchdir="$4"

    pushd $benchdir
    for bench in $benchs; do
        for class in $classes; do
            for nproc in $nprocs; do
                make $bench CLASS=$class NPROCS=$nproc
            done
        done
    done
    popd
}



do_bind_core_mpi()
{
    bench=$1
    class=$2
    nprocs=$3
    slots=$4
    bendir="$5"
    pname=$(./get_pname $bench $class $nprocs)
    outfile=$(./out_name mpi $bench $class $nprocs $slots)

    mpirun --bind-to-core --report-bindings -np $nprocs --slot-list $slots \
        "${bendir}/bin/${pname}" &> "${bendir}/output/${outfile}"
}


