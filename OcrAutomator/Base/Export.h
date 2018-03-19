#pragma once

#ifdef WIN32
	#ifdef LIB_EXPORT // inside DLL
		#define EXPOSE __declspec(dllexport)
	#else // outside DLL
		#define EXPOSE   __declspec(dllimport)
	#endif  // XYZLIBRARY_EXPORT
#else
	#define EXPOSE 
#endif