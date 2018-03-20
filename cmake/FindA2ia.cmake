find_path(A2ia_INCLUDE_DIR A2iATextReader.h $ENV{PROGRAMFILES}/A2iA/A2iA\ TextReader\ V5.0/Interface)
find_path(A2ia_LIBRARY A2iATextReader.lib $ENV{PROGRAMFILES}/A2iA/A2iA\ TextReader\ V5.0/Interface)
find_path(A2ia_DLL A2iATextReader.dll $ENV{PROGRAMFILES}/A2iA/A2iA\ TextReader\ V5.0/Interface)

add_library(A2ialib SHARED IMPORTED)
set_target_properties(A2ialib PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "A2ia_DLL"
  INTERFACE_INCLUDE_DIRECTORIES "${A2ia_INCLUDE_DIR}"
  
  IMPORTED_IMPLIB "${A2ia_LIBRARY}/A2iATextReader.lib"
  IMPORTED_LOCATION "${A2ia_LIBRARY}/A2iATextReader.dll"
)

mark_as_advanced(A2ia_LIBRARY A2ia_INCLUDE_DIR A2ia_DLL)