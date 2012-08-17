{
    "targets": [
        {
            "target_name": "clang-rw",

            "sources": [
                "./src/binding.cc",
                "./src/clang-rw.cc"
            ],

            # Throw in all of llvm-config's #defines
            # TODO: make this work with <!(llvm-config --cppflags)>
            "defines": [
                "NDEBUG",
                "_GNU_SOURCE",
                "__STDC_CONSTANT_MACROS",
                "__STDC_FORMAT_MACROS",
                "__STDC_LIMIT_MACROS"
            ],

            # Throw in all of llvm-config's CXX flags 
            # TODO: make this work with <!(llvm-config --cxxflags)>
            "cflags": [

            ],

            # Enable C++ exceptions (node-gyp has them off by default)
            "cflags!": [ "fno-exceptions" ],
            "cflags_cc!": [ "fno-exceptions" ],

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
            },

            "conditions": [
                ["OS=='mac'", {
                    "defines+": ["OSX"],
                    "link_settings": {
                        "libraries+": ["-lclangEdit"]
                    },
                    "xcode_settings": {"GCC_ENABLE_CPP_EXCEPTIONS": "YES"}
                }],
            ]
        }
    ]
}
