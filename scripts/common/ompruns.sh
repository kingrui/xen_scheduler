#!/bin/bash


do_run_free_omp()
{
    bench=$1
    class=$2
    nprocs=$3 
    bendir="$4"

    pname=$(./get_pname $bench $class)
    out_file=$(./out_name omp $bench $class $nprocs free)

    export OMP_NUM_THREADS=$nprocs
    "${bendir}/bin/${pname}" &> "${bendir}/output/${out_file}"
}

do_run_bind_socket_omp()
{
    bench=$1
    class=$2
    nprocs=$3
    bendir="$4"
    mask=$6
    pname=$(./get_pname $bench $class)
    out_file=$(./out_name omp $bench $class $nprocs "bind-socket")

    export OMP_NUM_THREADS=$nprocs
    echo "Affinity mask: $mask" > "${bendir}/output/${out_file}"
    taskset $mask "${bendir}/bin/${pname}" &> "${bendir}/output/${out_file}"
}

do_run_dual_bind_socket_omp()
{
    obench=$1
    ibench=$2
    class=$3
    nprocs=$4
    bendir="$5"
    mask=$6
    flag=0
    ob_pname=$(./get_pname $obench $class)
    ib_pname=$(./get_pname $ibench $class)
    ob_out=$(./out_name omp "$obench-$ibench" $class $nprocs "bind-socket")
    ib_out=$(./out_name omp "$ibench-$obench" $class $nprocs "bind-socket")
    
    export OMP_NUM_THREADS=$nprocs
    echo "Affinity mask: $mask" > "${bendir}/output/${ob_out}"
    taskset $mask "${bendir}/bin/${ib_pname}" &>/dev/null &
    taskset $mask "${bendir}/bin/${ob_pname}" &>"${bendir}/output/${ob_out}" & 
    ID=$!
    sleep 1
    while [ $(count_running $ob_pname) -ge 1 ]; do
        if need_restart $ib_pname $ob_pname; then
            echo "Restarting $ib_pname" > /dev/stderr
            taskset $mask "${bendir}/bin/${ib_pname}" &>/dev/null &
        fi
        sleep 1
    done
    echo "loop ends" > /dev/stderr
    # wait benchmark
    echo "waiting $ID" > /dev/stderr
    wait
    # clean up
    #test -z $(jobs -p) || kill $(jobs -p)
}

do_run_cross_omp()
{
    obench=$1
    ibench=$2
    class=$3
    nprocs=$4
    bendir="$5"
    ocpu=$6
    icpu=$7
    omem=$8
    imem=$9
    ob_pname=$(./get_pname $obench $class)
    ib_pname=$(./get_pname $ibench $class)
    ob_path="${bendir}/bin/${ob_pname}"
    ib_path="${bendir}/bin/${ib_pname}"
    # mem-this-that cpu-this-that
    ob_out=$(./out_name omp "$obench-$ibench" $class $nprocs \
        "mem$omem-$imem-cpu$ocpu-$icpu")
    ib_out=$(./out_name omp "$ibench-$obench" $class $nprocs \
        "mem$imem-$omem-cpu$icpu-$ocpu")
    counter=0
    export OMP_NUM_THREADS=$nprocs

    numactl --membind=$imem --physcpubind=$((4 + icpu)),$((6 + icpu)) \
        "$ib_path" &> "${bendir}/output/${ib_out}" &
    numactl --membind=$omem --physcpubind=$((0 + ocpu)),$((2 + ocpu)) \
        "$ob_path" &> "${bendir}/output/${ob_out}" &

    sleep 1
    while [ $(count_running $ob_pname) -ge 1 ]; do
        if need_restart $ib_pname $ob_pname; then
            counter=$((counter + 1))
            echo "Restarting $ib_pname" > /dev/stderr
            numactl --membind=$imem --physcpubind=$((4 + icpu)),$((6 + icpu)) \
                "$ib_pname" &> "${bendir}/output/$(./out_name omp $ibench-$obench\
                    $class $nprocs mem$imem-$omem-cpu$icpu-$ocpu-$counter)" &
        fi
        sleep 1
    done
    echo "waiting" > /dev/stderr
    wait
}

do_gen_run_file_omp()
{
    benchs="$1"
    class=$2
    nprocs=$3
    bendir="$4"
    dst="$5"
    alg=$6

    touch "$dst"
    for bench in $benchs; do
        pname=$(./get_pname $bench $class)
        rundir="${bendir}/bin"
        out_file=$(./out_name omp $bench $class $nprocs clavis-$alg)
        out="${bendir}/output/${out_file}"
        echo "${bench} 5 ${rundir}/${pname} > ${out}" >> "$dst"
        echo "***rundir ${rundir}" >> "$dst"
    done
}
