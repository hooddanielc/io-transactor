{
  "targets": [
    {
      "target_name": "io_transactor",
      "type": "static_library",
      "sources": [
        "src/io_transactor.cc",
        "src/json.cc"
      ],
      "cflags": [
        "-std=c++11",
        "-stdlib=libc++"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "conditions": [
        ["OS==\"mac\"", {
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS" : [
              "-stdlib=libc++",
              "-std=c++11",
              "-mmacosx-version-min=10.7"
            ],
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES"
          },
        }],
      ]
    }
  ]
}