mkdir build
cd build

REM ----------------------------------------------------------------------
IF NOT DEFINED WITH_CPLEX (SET WITH_CPLEX="")
IF NOT DEFINED WITH_GUROBI (SET WITH_GUROBI="")
IF "%WITH_CPLEX%" == "0" (SET WITH_CPLEX="")
IF "%WITH_GUROBI%" == "0" (SET WITH_GUROBI="")

if "%WITH_CPLEX%" == "" (
	if "%WITH_GUROBI%" == "" (
		ECHO "Either WITH_CPLEX or WITH_GUROBI must be 1"
		exit 1
	)
	REM if we build with Gurobi, we need to configure the paths.
	REM MHT chooses gurobi first if that is configured.
	REM The GUROBI_ROOT_DIR should point to gurobiXYZ\win64
    SET CPLEX_ARGS=""
    SET GUROBI_ARGS=""
    SET GUROBI_ARGS="%GUROBI_ARGS% -DWITH_GUROBI=ON"
    SET GUROBI_ARGS="%GUROBI_ARGS% -DGUROBI_ROOT_DIR=%GUROBI_ROOT_DIR%"
    dir "C:\gurobi751\win64\lib\gurobi*.lib" /s/b | findstr gurobi[0-9][0-9].lib > gurobilib.tmp
    set /p GUROBI_LIB=< gurobilib.tmp
    ECHO "found gurobi lib %GUROBI_LIB%"
    SET GUROBI_ARGS="%GUROBI_ARGS% -DGUROBI_LIBRARY=%GUROBI_LIB%"
    SET GUROBI_ARGS="%GUROBI_ARGS% -DGUROBI_INCLUDE_DIR=%GUROBI_ROOT_DIR%\include"
    SET GUROBI_ARGS="%GUROBI_ARGS% -DGUROBI_CXX_LIBRARY=%GUROBI_ROOT_DIR%\lib\gurobi_c++mt2015.lib"
    SET SUFFIX="_with_gurobi"
)
else (
	REM CPLEX is found automatically if installed. 
	REM No idea what happens with two CPLEXinstallations, but for now we don't care.
    SET SUFFIX="_with_cplex"
)

REM ----------------------------------------------------------------------

set CONFIGURATION=Release

cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_PREFIX_PATH="%LIBRARY_PREFIX%" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%"  ^
    -DBOOST_ROOT="%LIBRARY%" ^
    -DCMAKE_CXX_FLAGS="-DBOOST_ALL_NO_LIB /EHsc" ^
    ^
    %GUROBI_ARGS% ^
    ^
    -DSUFFIX=%SUFFIX% ^
    -DWITH_PYTHON=ON ^
    -DPYTHON_EXECUTABLE=%PYTHON%

cmake --build . --target ALL_BUILD --config %CONFIGURATION%
if errorlevel 1 exit 1
cmake --build . --target INSTALL --config %CONFIGURATION%
if errorlevel 1 exit 1