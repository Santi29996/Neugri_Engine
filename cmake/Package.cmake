# Release folder required by the delivery:
#
#   NeugriEngine_v0.1/
#     NeugriEngine/          <- exe + dlls + Assets
#     README.md
#     LICENCE.md
#     Licences/               <- third party licenses
#
# Usage:  cmake --install <build-dir> --config Release --component Engine --prefix dist/NeugriEngine_v0.1

set(PACKAGE_EXE_DIR NeugriEngine)

install(TARGETS NeugriEngine RUNTIME DESTINATION ${PACKAGE_EXE_DIR} COMPONENT Engine)
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:NeugriEngine> DESTINATION ${PACKAGE_EXE_DIR} COMPONENT Engine)
else()
    install(FILES $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE:assimp> DESTINATION ${PACKAGE_EXE_DIR} COMPONENT Engine)
    set_target_properties(NeugriEngine PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Assets DESTINATION ${PACKAGE_EXE_DIR} COMPONENT Engine)
install(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION . COMPONENT Engine)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE.md DESTINATION . RENAME LICENCE.md COMPONENT Engine)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/Licences DESTINATION . COMPONENT Engine)
