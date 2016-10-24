These recipe files can build two different packages:

Package name        Python module
------------        ---------------------------------
multi-hypotheses-tracking-with-cplex    import multiHypoTracking_with_cplex as multiHypoTracking
multi-hypotheses-tracking-with-gurobi   import multiHypoTracking_with_gurobi as multiHypoTracking

Configure your environment to select the variant to build.

Examples:

# Build multiHypoTracking-with-cplex
WITH_CPLEX=1 CPLEX_ROOT_DIR=/path/to/ibm/ILOG/CPLEX_Studio1251 conda build conda-recipe

# Build multiHypoTracking-with-gurobi
WITH_GUROBI=1 GUROBI_ROOT_DIR=/path/to/gurobi650/linux64 conda build conda-recipe