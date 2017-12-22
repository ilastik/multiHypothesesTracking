mkdir build
cd build

REM ----------------------------------------------------------------------
IF NOT DEFINED WITH_CPLEX (SET WITH_CPLEX="0")
IF NOT DEFINED WITH_GUROBI (SET WITH_GUROBI="0")

IF "%WITH_CPLEX%" == "0" (
	if "%WITH_GUROBI%" == "0" (
		ECHO "Either WITH_CPLEX or WITH_GUROBI must be 1"
		exit 1
	)
	IF NOT DEFINED GUROBI_ROOT_DIR (
		ECHO "GUROBI_ROOT_DIR must be set for building!"
		exit 1
	)
	IF "%GUROBI_ROOT_DIR%" == "" (
		ECHO "GUROBI_ROOT_DIR cannot be empty for building!"
		exit 1
	) ELSE (
		ECHO "Using GUROBI_ROOT_DIR=%GUROBI_ROOT_DIR%"
	)
	REM if we build with Gurobi, we need to configure the paths.
	REM MHT chooses gurobi first if that is configured.
	REM The GUROBI_ROOT_DIR should point to gurobiXYZ\win64
    REM dir "%GUROBI_ROOT_DIR%\lib\gurobi*.lib" /s/b|findstr gurobi[0-9][0-9].lib>gurobilib.tmp
    REM set /p GUROBI_LIB=<gurobilib.tmp
    ECHO found gurobi lib %GUROBI_LIB_WIN%
    SET OPTIMIZER_ARGS=-DWITH_GUROBI=ON -DGUROBI_ROOT_DIR=%GUROBI_ROOT_DIR% ^
      -DGUROBI_LIBRARY=%GUROBI_LIB_WIN% -DGUROBI_INCLUDE_DIR=%GUROBI_ROOT_DIR%\include ^
      -DGUROBI_CXX_LIBRARY=%GUROBI_ROOT_DIR%\lib\gurobi_c++md2015.lib
    SET SUFFIX=_with_gurobi
) ELSE (
    SET OPTIMIZER_ARGS=-DWITH_CPLEX=ON -DCPLEX_ROOT_DIR=%CPLEX_ROOT_DIR% -DCPLEX_WIN_VERSION=%CPLEX_WIN_VERSION%
    SET SUFFIX=_with_cplex
)

REM ----------------------------------------------------------------------

set CONFIGURATION=Release

cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_PREFIX_PATH="%LIBRARY_PREFIX%" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%"  ^
    -DBOOST_ROOT="%LIBRARY%" ^
    -DCMAKE_CXX_FLAGS="-DBOOST_ALL_NO_LIB /EHsc" ^
    ^
    %OPTIMIZER_ARGS% ^
    ^
    -DSUFFIX=%SUFFIX% ^
    -DWITH_PYTHON=ON ^
    -DPYTHON_EXECUTABLE=%PYTHON%

cmake --build . --target ALL_BUILD --config %CONFIGURATION%
if errorlevel 1 exit 1
cmake --build . --target INSTALL --config %CONFIGURATION%
if errorlevel 1 exit 1