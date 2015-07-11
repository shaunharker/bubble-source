# add the executable

set ( LIBS      ${LIBS} )

add_executable ( doxygen_xml_to_json doxygen_xml_to_json.cpp )
target_link_libraries ( doxygen_xml_to_json ${LIBS} )

install(TARGETS doxygen_xml_to_json
        RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin )
