set(PFP_TESTSUITES)
set(PFP_TEST_DEPS
  ${SANITIZER_COMMON_LIT_TEST_DEPS})


macro(add_pfp_testsuite arch lld thinlto compilerrt libunwind)
  set(PFP_TEST_TARGET_ARCH ${arch})
  get_test_cc_for_arch(${arch} PFP_TEST_TARGET_CC PFP_TEST_TARGET_CFLAGS)

  string(TOUPPER ${arch} CONFIG_NAME)

  if (${thinlto})
    set(CONFIG_NAME "thinlto-${CONFIG_NAME}")
    list(APPEND PFP_TEST_DEPS LTO)
  endif()
   if (${libunwind})
    set(CONFIG_NAME "libunwind-${CONFIG_NAME}")
    list(APPEND PFP_TEST_DEPS unwind)
  endif()
  if (${lld})
    set(CONFIG_NAME "lld-${CONFIG_NAME}")
    if (TARGET lld)
      list(APPEND PFP_TEST_DEPS lld)
    endif()
  endif()
  if (${compilerrt})
    set(CONFIG_NAME "compiler-rt-${CONFIG_NAME}")
    list(APPEND PFP_TEST_DEPS compiler-rt)
  endif()
  set(PFP_TEST_USE_THINLTO ${thinlto})
  set(PFP_TEST_USE_LLD ${lld})

  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}/lit.site.cfg.py)
  list(APPEND PFP_TESTSUITES ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME})
endmacro()

set(PFP_TEST_ARCH ${PFP_SUPPORTED_ARCH})


foreach(arch ${PFP_TEST_ARCH})
  if (arch STREQUAL ${X86_64})
    add_pfp_testsuite(${arch} False False False False)
  endif()
  if (arch STREQUAL ${ARM64})
    add_pfp_testsuite(${arch} True True True True)
  endif()
endforeach()

message(STATUS "PFP tests. " "Arch: ${PFP_TEST_ARCH} Deps: ${PFP_TEST_DEPS}")
add_lit_testsuite(check-pfp "Running the PointerFieldProtection tests"
  ${PFP_TESTSUITES}
  DEPENDS ${PFP_TEST_DEPS})

