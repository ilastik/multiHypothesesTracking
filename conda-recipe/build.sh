# remove trailing slash if there is one
GUROBI_ROOT_DIR=${GUROBI_ROOT_DIR%/}
echo "Passing GUROBI_ROOT_DIR with value ${GUROBI_ROOT_DIR}"
GUROBI_ARGS=""
GUROBI_ARGS="${GUROBI_ARGS} -DWITH_GUROBI=ON"
GUROBI_ARGS="${GUROBI_ARGS} -DGUROBI_ROOT_DIR=${GUROBI_ROOT_DIR}"
# since gurobi 801, there is a _light lib included, which we can link agains:
if ls ${GUROBI_ROOT_DIR}/lib/libgurobi*_light${SHLIB_EXT} 1> /dev/null 2>&1; then
    # Gurobi >= 801
    GUROBI_ARGS="${GUROBI_ARGS} -DGUROBI_LIBRARY=$(ls ${GUROBI_ROOT_DIR}/lib/libgurobi*_light${SHLIB_EXT})"
else
    # Gurobi < 801
    GUROBI_ARGS="${GUROBI_ARGS} -DGUROBI_LIBRARY=$(ls ${GUROBI_ROOT_DIR}/lib/libgurobi${SHLIB_EXT})"
fi
GUROBI_ARGS="${GUROBI_ARGS} -DGUROBI_INCLUDE_DIR=${GUROBI_ROOT_DIR}/include"
if [ $(uname) == "Darwin" ]; then
    # Note: For Mac, the nice Gurobi people provide two versions of the gurobi library,
    #       depending on which version of the C++ std library you need to use:
    #       - For libstdc++ (from the GNU people), use libgurobi_stdc++.a
    #       - For libc++    (from the clang people), use libgurobi_c++.a
    #       We use clang now so we use the libc++ version.
    GUROBI_ARGS="${GUROBI_ARGS} -DGUROBI_CXX_LIBRARY=${GUROBI_ROOT_DIR}/lib/libgurobi_c++.a"
else
    # seems that at least gurobi1200 does also provide ibgurobi_c++.a (linking to libgurobi_g++8.5.a)
    GUROBI_ARGS="${GUROBI_ARGS} -DGUROBI_CXX_LIBRARY=${GUROBI_ROOT_DIR}/lib/libgurobi_c++.a"
fi

SUFFIX="_with_gurobi"


##
## START THE BUILD
##

mkdir build
cd build

if [ $(uname) == "Darwin" ]; then
    CXXFLAGS="-std=c++11 -stdlib=libc++"
    LDFLAGS="-undefined dynamic_lookup ${LDFLAGS}"
    LDFLAGS="-L${PREFIX}/lib ${LDFLAGS}"
else
    LDFLAGS="-Wl,-rpath-link,${PREFIX}/lib -L${PREFIX}/lib"
fi

CXXFLAGS="${CXXFLAGS} -I${PREFIX}/include"

##
## Configure
##
cmake .. \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 \
        -DCMAKE_INSTALL_PREFIX=${PREFIX} \
        -DCMAKE_PREFIX_PATH=${PREFIX} \
\
        -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS}" \
        -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
        -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
        -DCMAKE_CXX_FLAGS_RELEASE="${CXXFLAGS}" \
        -DCMAKE_CXX_FLAGS_DEBUG="${CXXFLAGS}" \
\
        -DBoost_INCLUDE_DIR=${PREFIX}/include \
        -DBoost_LIBRARY_DIRS=${PREFIX}/lib \
        -DBoost_PYTHON_LIBRARY=${PREFIX}/lib/libboost_python${CONDA_PY}${SHLIB_EXT} \
        -DBoost_PYTHON_LIBRARY_RELEASE=${PREFIX}/lib/libboost_python${CONDA_PY}${SHLIB_EXT} \
        -DBoost_PYTHON_LIBRARY_DEBUG=${PREFIX}/lib/libboost_python${CONDA_PY}${SHLIB_EXT} \
        -DBoost_NO_BOOST_CMAKE=ON \
        ${GUROBI_ARGS} \
        -DSUFFIX=${SUFFIX} \
\
        -DWITH_PYTHON=ON \
        -DPYTHON_EXECUTABLE=${PYTHON} \
\
        -DCMAKE_BUILD_TYPE=Release \
##

##
## Compile
##
make -j${CPU_COUNT}
make install

MHT_LIB_SO=${PREFIX}/lib/libmultiHypoTracking${SUFFIX}${SHLIB_EXT}
MHT_PYMODULE_SO=${SP_DIR}/multiHypoTracking${SUFFIX}.so



if [ `uname` == "Darwin" ]; then
    # Set install name according using @rpath
    # since gurobi 801, there is a _light lib included, which we can link agains:
    if ls ${GUROBI_ROOT_DIR}/lib/libgurobi*_light${SHLIB_EXT}* 1> /dev/null 2>&1; then
        # Gurobi >= 801
        fullpath=$(ls ${GUROBI_ROOT_DIR}/lib/libgurobi*_light${SHLIB_EXT}*)
    else
        # Gurobi < 801
        fullpath=$(ls ${GUROBI_ROOT_DIR}/lib/libgurobi*${SHLIB_EXT}*)
    fi
    install_name_tool -change $fullpath @rpath/$(basename $fullpath) ${MHT_LIB_SO} 
    install_name_tool -change $fullpath @rpath/$(basename $fullpath) ${MHT_PYMODULE_SO}
fi


