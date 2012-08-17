{
    "targets": [
        {
            "target_name": "clang-rw",

            "sources": [
                "./src/binding.cc",
                "./src/clang-rw.cc"
            ],

            "cflags": [ "<!@(llvm-config --cxxflags)" ],

            # Enable C++ exceptions (node-gyp has them off by default)
            "cflags!": [ "-fno-exceptions" ],
            "cflags_cc!": [ "-fno-exceptions" ],

            "link_settings": {
                "ldflags": [ "<!@(llvm-config --ldflags)" ],
                "libraries": [ "<!@(llvm-config --libs cppbackend)" ],
                "libraries+": [
                    "-lclangFrontend",
                    "-lclangDriver",
                    "-lclangSerialization",
                    "-lclangParse",
                    "-lclangSema",
                    "-lclangAnalysis",
                    "-lclangRewrite",
                    "-lclangAST",
                    "-lclangLex",
                    "-lclangBasic",
                ]
            },

            "conditions": [
                ["OS=='mac'", {
                    # gyp doesn't like llvm-config on my Mac...
                    # must be my custom install...
                    "defines": [
                        "__STDC_CONSTANT_MACROS",
                        "__STDC_FORMAT_MACROS",
                        "__STDC_LIMIT_MACROS",
                        "OSX"
                    ],

                    "cflags": [

                    ],

                    "link_settings": { "libraries+": [ "-lclangEdit" ] },

                    "xcode_settings": { "GCC_ENABLE_CPP_EXCEPTIONS": "YES" }
                }]
            ]
        }
    ]
}
