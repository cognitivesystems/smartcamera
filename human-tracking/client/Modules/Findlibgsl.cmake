FIND_PATH(LIBGSL_INCLUDES gsl/gsl_version.h
	${CMAKE_INCLUDE_PATH}
	$ENV{LIBGSLDIR}/include
	/usr/local/libgsl/include
	/usr/local/include
	/usr/include
	c:/siftgpu/include
	$ENV{SIFTGPUPATH}/include
	$ENV{ProgramFiles}/siftgpu/include
)

FIND_LIBRARY(LIBGSL_GSL_LIBRARY
	NAMES gsl
	PATHS
	${CMAKE_LIBRARY_PATH}
	$ENV{HOME}/lib
	/usr/local/lib
	/usr/lib
	c:/siftgpu/lib
	$ENV{SIFTGPUPATH}/lib
	$ENV{ProgramFiles}/siftgpu/lib
)

IF (WIN32)
	SET(
		LIBGSL_LIBRARIES
		${LIBGSL_GSL_LIBRARY}
	)

	MARK_AS_ADVANCED(
		LIBGSL_INCLUDES
		LIBGSL_LIBRARIES
		LIBGSL_GSL_LIBRARY
	) 
ELSE (WIN32)
	FIND_LIBRARY(LIBGSL_GSLCBLAS_LIBRARY
		NAMES gslcblas
		PATHS
		${CMAKE_LIBRARY_PATH}
		$ENV{HOME}/lib
		/usr/local/lib
		/usr/lib
		c:/siftgpu/lib
		$ENV{SIFTGPUPATH}/lib
		$ENV{ProgramFiles}/siftgpu/lib
	)

	SET(
		LIBGSL_LIBRARIES
		${LIBGSL_GSL_LIBRARY}
		${LIBGSL_GSLCBLAS_LIBRARY}
	)

	MARK_AS_ADVANCED(
		LIBGSL_INCLUDES
		LIBGSL_LIBRARIES
		LIBGSL_GSL_LIBRARY
		LIBGSL_GSLCBLAS_LIBRARY
	) 
ENDIF (WIN32)

