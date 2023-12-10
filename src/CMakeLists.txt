find_package(LLVM 12.0.0 REQUIRED CONFIG)

add_executable (gsm
  GSM.cpp
  CodeGen.cpp
  Lexer.cpp
  Parser.cpp
  Sema.cpp
  )
target_link_libraries(gsm PRIVATE ${llvm_libs})
