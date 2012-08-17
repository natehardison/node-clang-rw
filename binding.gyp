{
    "targets": [
        {
            "target_name": "clang-rw",

            "sources": [
                "./src/binding.cc",
                "./src/clang-rw.cc"
            ],

            # TODO!
            "cxxflags": [
#                "-fnortti",
            ],

            "defines": [
                "__STDC_CONSTANT_MACROS",
                "__STDC_FORMAT_MACROS",
                "__STDC_LIMIT_MACROS"
            ],

            "link_settings": {
                "libraries": [
                    "-lclangAnalysis",
                    "-lclangAST",
                    "-lclangBasic",
                    "-lclangFrontend",
                    "-lclangDriver",
                    "-lclangLex",
                    "-lclangParse",
                    "-lclangRewrite",
                    "-lclangSema",
                    "-lclangSerialization",
                    "-lLLVMMC",
                    "-lLLVMSupport"
                ]
            }
        }
    ]
}
