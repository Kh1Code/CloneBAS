set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

add_compile_definitions(_WIN32_WINNT=0x0A00)
set(CMAKE_CXX_FLAGS "/EHsc ${CMAKE_CXX_FLAGS}")

add_clang_tool(clang-cppana
  Main.cpp
  CloneSet.cpp
  EigenWord.cpp
  MurmurHash3.cpp
  SimHashConfig.cpp
  SimHasher.cpp
  EigenWordGenerate.cpp
  FixedChecker.cpp
  FixedNode.cpp
  NodeTravesal.cpp
  ScoreController.cpp
  ScoreUnit.cpp
  EventCenter.cpp
  EventClient.cpp
  InfoStruct.cpp
  InfoController.cpp
  Log.cpp
  AnaHeader.h
  FingerPrintGenerator.cpp
 )

clang_target_link_libraries(clang-cppana
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangSerialization
  clangStaticAnalyzerFrontend
  clangTooling
  clangToolingSyntax
  clangLex
  clangASTMatchers
  clangToolingRefactoring
  libboost_json-vc142-mt-x64-1_81.lib
  LLVMSupport
  )