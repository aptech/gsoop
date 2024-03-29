{
  "targets": [
    {
      "target_name": "ge",
      "include_dirs": ["src", "include", "."],
      "cflags!": ['-fno-exceptions', '-std=c++11'], 
      "cflags_cc!": ['-fno-exceptions', '-std=c++11'],
      'defines': [
          'GAUSS_LIBRARY','SWIGJAVASCRIPT'
      ],
      "sources": ["src/gauss.cpp", "src/gematrix.cpp", "src/gearray.cpp", "src/gestringarray.cpp", "src/geworkspace.cpp", "src/workspacemanager.cpp", "src/gesymbol.cpp", "node/gauss_wrap.cpp"],
      "conditions": [
        ["OS=='win'", {
          "libraries": [
              "-L<!(echo %MTENGHOME%)",
              "-lmteng"
          ]
        }],
        ["OS!='win'", {
          "libraries": [
              "-L<!(echo $MTENGHOME)",
              "-lmteng"
          ]
        }],
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
          }
        }]
      ]
    }
  ]
}
