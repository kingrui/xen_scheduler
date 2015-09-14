if test $# -ne 2
	then echo "usage:sh runall.sh {process num} {class}."
	exit 1
fi
np=$2
class=$1

for cmd in bt cg ep ft is lu mg sp
do
	if [ -x $cmd.$np.$class ]
		then mpirun --hostfile /root/mpd.host -np 16 $cmd.$np.$class >$cmd.rst
	fi
done

