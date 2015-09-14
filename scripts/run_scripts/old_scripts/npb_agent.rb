class NPBAgent

  NPROCS = 'nprocs'
  BENTYPE = 'bentype'
  BENNAME = 'benname'
  CLASS = 'class'

  def get_cmd(config)
    cmds = { 'mpi' => "mpirun -np #{config[NPROCS]} #{File.join(@dirs[config[BENTYPE]],config[BENNAME])}.#{config[CLASS]}.#{config[NPROCS]}",
             'omp' => "
    }
  end

  def initialize(config)
    @dirs = { 'mpi' => config['mpi-dir'],
              'omp' => config['omp-dir']
    }
  end

  def run_togather(run_config)

  end
end
