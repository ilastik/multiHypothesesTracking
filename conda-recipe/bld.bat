mkdir build
cd build

REM ----------------------------------------------------------------------
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
REM The GUROBI_ROOT_DIR should point to gurobiXYZ\win64
SET OPTIMIZER_ARGS=-DWITH_GUROBI=ON -DGUROBI_ROOT_DIR=%GUROBI_ROOT_DIR%
SET SUFFIX=_with_gurobi

REM ----------------------------------------------------------------------

set CONFIGURATION=Release

cmake .. -G "NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DCMAKE_PREFIX_PATH="%LIBRARY_PREFIX%" ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%"  ^
    -DCMAKE_CXX_FLAGS="-DBOOST_ALL_NO_LIB /EHsc" ^
    ^
    %OPTIMIZER_ARGS% ^
    ^
    -DSUFFIX=%SUFFIX% ^
    -DWITH_PYTHON=ON ^
    -DPYTHON_EXECUTABLE:FILEPATH=%PYTHON% ^
    -DBoost_NO_BOOST_CMAKE=ON

nmake all
if errorlevel 1 exit 1
nmake install
if errorlevel 1 exit 1
