# Copyright 2012 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{
  'variables': {
    'icu_use_data_file_flag%': 0,
    'v8_code': 1,
    'v8_random_seed%': 314159265,
    'v8_vector_stores%': 0,
    'embed_script%': "",
    'v8_extra_library_files%': [],
    'v8_experimental_extra_library_files%': [],
    'mksnapshot_exec': '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)mksnapshot<(EXECUTABLE_SUFFIX)',
  },
  'includes': ['../../build/toolchain.gypi', '../../build/features.gypi'],
  'conditions': [
    ['OS=="mac"', {
      'xcode_settings': {
        'SKIP_INSTALL': 'YES'
      }
    }],
    ['OS=="win"', {
      'variables': {
        'gyp_generators': '<!(echo $GYP_GENERATORS)',
      },
      'msvs_disabled_warnings': [4351, 4355, 4800],
      'cflags': [
        '/Zc:sizedDealloc-'
      ]
    }],
  ],

  'targets': [
    {
      'target_name': 'v8',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'dependencies_traverse': 1,
      'dependencies': ['v8_maybe_snapshot'],
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
        }, {
          'toolsets': ['target'],
        }],
        ['component=="shared_library"', {
          'type': '<(component)',
          'sources': [
            # Note: on non-Windows we still build this file so that gyp
            # has some sources to link into the component.
            '../../src/v8dll-main.cc',
          ],
          'include_dirs': [
            '../..',
          ],
          'defines': [
            'V8_SHARED',
            'BUILDING_V8_SHARED',
          ],
          'direct_dependent_settings': {
            'defines': [
              'V8_SHARED',
              'USING_V8_SHARED',
            ],
          },
          'target_conditions': [
            ['OS=="android" and _toolset=="target"', {
              'libraries': [
                '-llog',
              ],
              'include_dirs': [
                'src/common/android/include',
              ],
            }],
          ],
          'conditions': [
            ['OS=="mac"', {
              'xcode_settings': {
                'OTHER_LDFLAGS': ['-dynamiclib', '-all_load']
              },
            }],
            ['soname_version!=""', {
              'product_extension': 'so.<(soname_version)',
            }],
          ],
        },
        {
          'type': 'none',
        }],
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../include',
        ],
      },
    },
    {
      # This rule delegates to either v8_snapshot, v8_nosnapshot, or
      # v8_external_snapshot, depending on the current variables.
      # The intention is to make the 'calling' rules a bit simpler.
      'target_name': 'v8_maybe_snapshot',
      'type': 'none',
      'conditions': [
        ['v8_use_snapshot!="true"', {
          # The dependency on v8_base should come from a transitive
          # dependency however the Android toolchain requires libv8_base.a
          # to appear before libv8_snapshot.a so it's listed explicitly.
          'dependencies': ['v8_base', 'v8_nosnapshot'],
        }],
        ['v8_use_snapshot=="true" and v8_use_external_startup_data==0', {
          # The dependency on v8_base should come from a transitive
          # dependency however the Android toolchain requires libv8_base.a
          # to appear before libv8_snapshot.a so it's listed explicitly.
          'dependencies': ['v8_base', 'v8_snapshot'],
        }],
        ['v8_use_snapshot=="true" and v8_use_external_startup_data==1 and want_separate_host_toolset==0', {
          'dependencies': ['v8_base', 'v8_external_snapshot'],
          'inputs': ['<(PRODUCT_DIR)/snapshot_blob.bin'],
          'conditions': [
            ['v8_separate_ignition_snapshot==1', {
              'inputs': ['<(PRODUCT_DIR)/snapshot_blob_ignition.bin'],
            }],
          ]
        }],
        ['v8_use_snapshot=="true" and v8_use_external_startup_data==1 and want_separate_host_toolset==1', {
          'dependencies': ['v8_base', 'v8_external_snapshot'],
          'target_conditions': [
            ['_toolset=="host"', {
              'inputs': ['<(PRODUCT_DIR)/snapshot_blob_host.bin'],
            }, {
              'inputs': ['<(PRODUCT_DIR)/snapshot_blob.bin'],
            }],
          ],
          'conditions': [
            ['v8_separate_ignition_snapshot==1', {
              'target_conditions': [
                ['_toolset=="host"', {
                  'inputs': ['<(PRODUCT_DIR)/snapshot_blob_ignition_host.bin'],
                }, {
                  'inputs': ['<(PRODUCT_DIR)/snapshot_blob_ignition.bin'],
                }],
              ],
            }],
          ],
        }],
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
        }, {
          'toolsets': ['target'],
        }],
      ]
    },
    {
      'target_name': 'v8_snapshot',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'static_library',
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
          'dependencies': [
            'mksnapshot#host',
            'js2c#host',
          ],
        }, {
          'toolsets': ['target'],
          'dependencies': [
            'mksnapshot',
            'js2c',
          ],
        }],
        ['component=="shared_library"', {
          'defines': [
            'V8_SHARED',
            'BUILDING_V8_SHARED',
          ],
          'direct_dependent_settings': {
            'defines': [
              'V8_SHARED',
              'USING_V8_SHARED',
            ],
          },
        }],
      ],
      'dependencies': [
        'v8_base',
      ],
      'include_dirs+': [
        '../..',
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/libraries.cc',
        '<(SHARED_INTERMEDIATE_DIR)/experimental-libraries.cc',
        '<(SHARED_INTERMEDIATE_DIR)/extras-libraries.cc',
        '<(SHARED_INTERMEDIATE_DIR)/experimental-extras-libraries.cc',
        '<(INTERMEDIATE_DIR)/snapshot.cc',
      ],
      'actions': [
        {
          'action_name': 'run_mksnapshot',
          'inputs': [
            '<(mksnapshot_exec)',
            '<(embed_script)',
          ],
          'outputs': [
            '<(INTERMEDIATE_DIR)/snapshot.cc',
          ],
          'variables': {
            'mksnapshot_flags': [
              '--log-snapshot-positions',
              '--logfile', '<(INTERMEDIATE_DIR)/snapshot.log',
            ],
            'conditions': [
              ['v8_random_seed!=0', {
                'mksnapshot_flags': ['--random-seed', '<(v8_random_seed)'],
              }],
              ['v8_vector_stores!=0', {
                'mksnapshot_flags': ['--vector-stores'],
              }],
            ],
          },
          'action': [
            '<(mksnapshot_exec)',
            '<@(mksnapshot_flags)',
            '--startup_src', '<@(INTERMEDIATE_DIR)/snapshot.cc',
            '<(embed_script)',
          ],
        },
      ],
    },
    {
      'target_name': 'v8_nosnapshot',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'static_library',
      'dependencies': [
        'v8_base',
      ],
      'include_dirs+': [
        '../..',
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/libraries.cc',
        '<(SHARED_INTERMEDIATE_DIR)/experimental-libraries.cc',
        '<(SHARED_INTERMEDIATE_DIR)/extras-libraries.cc',
        '<(SHARED_INTERMEDIATE_DIR)/experimental-extras-libraries.cc',
        '../../src/snapshot/snapshot-empty.cc',
      ],
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
          'dependencies': ['js2c#host'],
        }, {
          'toolsets': ['target'],
          'dependencies': ['js2c'],
        }],
        ['component=="shared_library"', {
          'defines': [
            'BUILDING_V8_SHARED',
            'V8_SHARED',
          ],
        }],
      ]
    },
    {
      'target_name': 'v8_external_snapshot',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'static_library',
      'conditions': [
        [ 'v8_use_external_startup_data==1', {
          'conditions': [
            ['want_separate_host_toolset==1', {
              'toolsets': ['host', 'target'],
              'dependencies': [
                'mksnapshot#host',
                'js2c#host',
                'natives_blob',
            ]}, {
              'toolsets': ['target'],
              'dependencies': [
                'mksnapshot',
                'js2c',
                'natives_blob',
              ],
            }],
            ['component=="shared_library"', {
              'defines': [
                'V8_SHARED',
                'BUILDING_V8_SHARED',
              ],
              'direct_dependent_settings': {
                'defines': [
                  'V8_SHARED',
                  'USING_V8_SHARED',
                ],
              },
            }],
            # Extra snapshot blob for ignition.
            ['v8_separate_ignition_snapshot==1', {
              # This is concatenated to the other actions list of
              # v8_external_snapshot.
              'actions': [
                {
                  'action_name': 'run_mksnapshot (ignition)',
                  'inputs': ['<(mksnapshot_exec)'],
                  'variables': {
                    # TODO: Extract common mksnapshot_flags to a separate
                    # variable.
                    'mksnapshot_flags_ignition': [
                      '--ignition',
                      '--log-snapshot-positions',
                      '--logfile', '<(INTERMEDIATE_DIR)/snapshot_ignition.log',
                    ],
                    'conditions': [
                      ['v8_random_seed!=0', {
                        'mksnapshot_flags_ignition': ['--random-seed', '<(v8_random_seed)'],
                      }],
                      ['v8_vector_stores!=0', {
                        'mksnapshot_flags_ignition': ['--vector-stores'],
                      }],
                    ],
                  },
                  'conditions': [
                    ['want_separate_host_toolset==1', {
                      'target_conditions': [
                        ['_toolset=="host"', {
                          'outputs': ['<(PRODUCT_DIR)/snapshot_blob_ignition_host.bin'],
                          'action': [
                            '<(mksnapshot_exec)',
                            '<@(mksnapshot_flags_ignition)',
                            '--startup_blob', '<(PRODUCT_DIR)/snapshot_blob_ignition_host.bin',
                            '<(embed_script)',
                          ],
                        }, {
                          'outputs': ['<(PRODUCT_DIR)/snapshot_blob_ignition.bin'],
                          'action': [
                            '<(mksnapshot_exec)',
                            '<@(mksnapshot_flags_ignition)',
                            '--startup_blob', '<(PRODUCT_DIR)/snapshot_blob_ignition.bin',
                            '<(embed_script)',
                          ],
                        }],
                      ],
                    }, {
                      'outputs': ['<(PRODUCT_DIR)/snapshot_blob_ignition.bin'],
                      'action': [
                        '<(mksnapshot_exec)',
                        '<@(mksnapshot_flags_ignition)',
                        '--startup_blob', '<(PRODUCT_DIR)/snapshot_blob_ignition.bin',
                        '<(embed_script)',
                      ],
                    }],
                  ],
                },
              ],
            }],
          ],
          'dependencies': [
            'v8_base',
          ],
          'include_dirs+': [
            '../..',
          ],
          'sources': [
            '../../src/snapshot/natives-external.cc',
            '../../src/snapshot/snapshot-external.cc',
          ],
          'actions': [
            {
              'action_name': 'run_mksnapshot (external)',
              'inputs': ['<(mksnapshot_exec)'],
              'variables': {
                'mksnapshot_flags': [
                  '--log-snapshot-positions',
                  '--logfile', '<(INTERMEDIATE_DIR)/snapshot.log',
                ],
                'conditions': [
                  ['v8_random_seed!=0', {
                    'mksnapshot_flags': ['--random-seed', '<(v8_random_seed)'],
                  }],
                  ['v8_vector_stores!=0', {
                    'mksnapshot_flags': ['--vector-stores'],
                  }],
                ],
              },
              'conditions': [
                ['want_separate_host_toolset==1', {
                  'target_conditions': [
                    ['_toolset=="host"', {
                      'outputs': ['<(PRODUCT_DIR)/snapshot_blob_host.bin'],
                      'action': [
                        '<(mksnapshot_exec)',
                        '<@(mksnapshot_flags)',
                        '--startup_blob', '<(PRODUCT_DIR)/snapshot_blob_host.bin',
                        '<(embed_script)',
                      ],
                    }, {
                      'outputs': ['<(PRODUCT_DIR)/snapshot_blob.bin'],
                      'action': [
                        '<(mksnapshot_exec)',
                        '<@(mksnapshot_flags)',
                        '--startup_blob', '<(PRODUCT_DIR)/snapshot_blob.bin',
                        '<(embed_script)',
                      ],
                    }],
                  ],
                }, {
                  'outputs': ['<(PRODUCT_DIR)/snapshot_blob.bin'],
                  'action': [
                    '<(mksnapshot_exec)',
                    '<@(mksnapshot_flags)',
                    '--startup_blob', '<(PRODUCT_DIR)/snapshot_blob.bin',
                    '<(embed_script)',
                  ],
                }],
              ],
            },
          ],
        }],
      ],
    },
    {
      'target_name': 'v8_base',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'static_library',
      'dependencies': [
        'v8_libbase',
      ],
      'variables': {
        'optimize': 'max',
      },
      'include_dirs+': [
        '../..',
        # To be able to find base/trace_event/common/trace_event_common.h
        '../../..',
      ],
      'defines': [
        # TODO(jochen): Remove again after this is globally turned on.
        'V8_IMMINENT_DEPRECATION_WARNINGS',
      ],
      'sources': [  ### gcmole(all) ###
        '../../include/v8-debug.h',
        '../../include/v8-experimental.h',
        '../../include/v8-platform.h',
        '../../include/v8-profiler.h',
        '../../include/v8-testing.h',
        '../../include/v8-util.h',
        '../../include/v8-version.h',
        '../../include/v8.h',
        '../../include/v8config.h',
        '../../src/accessors.cc',
        '../../src/accessors.h',
        '../../src/address-map.cc',
        '../../src/address-map.h',
        '../../src/allocation.cc',
        '../../src/allocation.h',
        '../../src/allocation-site-scopes.cc',
        '../../src/allocation-site-scopes.h',
        '../../src/api-experimental.cc',
        '../../src/api-experimental.h',
        '../../src/api.cc',
        '../../src/api.h',
        '../../src/api-natives.cc',
        '../../src/api-natives.h',
        '../../src/arguments.cc',
        '../../src/arguments.h',
        '../../src/assembler.cc',
        '../../src/assembler.h',
        '../../src/assert-scope.h',
        '../../src/assert-scope.cc',
        '../../src/ast/ast-expression-rewriter.cc',
        '../../src/ast/ast-expression-rewriter.h',
        '../../src/ast/ast-expression-visitor.cc',
        '../../src/ast/ast-expression-visitor.h',
        '../../src/ast/ast-literal-reindexer.cc',
        '../../src/ast/ast-literal-reindexer.h',
        '../../src/ast/ast-numbering.cc',
        '../../src/ast/ast-numbering.h',
        '../../src/ast/ast-value-factory.cc',
        '../../src/ast/ast-value-factory.h',
        '../../src/ast/ast.cc',
        '../../src/ast/ast.h',
        '../../src/ast/modules.cc',
        '../../src/ast/modules.h',
        '../../src/ast/prettyprinter.cc',
        '../../src/ast/prettyprinter.h',
        '../../src/ast/scopeinfo.cc',
        '../../src/ast/scopeinfo.h',
        '../../src/ast/scopes.cc',
        '../../src/ast/scopes.h',
        '../../src/ast/variables.cc',
        '../../src/ast/variables.h',
        '../../src/atomic-utils.h',
        '../../src/background-parsing-task.cc',
        '../../src/background-parsing-task.h',
        '../../src/bailout-reason.cc',
        '../../src/bailout-reason.h',
        '../../src/basic-block-profiler.cc',
        '../../src/basic-block-profiler.h',
        '../../src/bignum-dtoa.cc',
        '../../src/bignum-dtoa.h',
        '../../src/bignum.cc',
        '../../src/bignum.h',
        '../../src/bit-vector.cc',
        '../../src/bit-vector.h',
        '../../src/bootstrapper.cc',
        '../../src/bootstrapper.h',
        '../../src/builtins.cc',
        '../../src/builtins.h',
        '../../src/cached-powers.cc',
        '../../src/cached-powers.h',
        '../../src/cancelable-task.cc',
        '../../src/cancelable-task.h',
        '../../src/char-predicates.cc',
        '../../src/char-predicates-inl.h',
        '../../src/char-predicates.h',
        '../../src/checks.h',
        '../../src/code-factory.cc',
        '../../src/code-factory.h',
        '../../src/code-stubs.cc',
        '../../src/code-stubs.h',
        '../../src/code-stubs-hydrogen.cc',
        '../../src/codegen.cc',
        '../../src/codegen.h',
        '../../src/compilation-cache.cc',
        '../../src/compilation-cache.h',
        '../../src/compilation-dependencies.cc',
        '../../src/compilation-dependencies.h',
        '../../src/compilation-statistics.cc',
        '../../src/compilation-statistics.h',
        '../../src/compiler/access-builder.cc',
        '../../src/compiler/access-builder.h',
        '../../src/compiler/access-info.cc',
        '../../src/compiler/access-info.h',
        '../../src/compiler/all-nodes.cc',
        '../../src/compiler/all-nodes.h',
        '../../src/compiler/ast-graph-builder.cc',
        '../../src/compiler/ast-graph-builder.h',
        '../../src/compiler/ast-loop-assignment-analyzer.cc',
        '../../src/compiler/ast-loop-assignment-analyzer.h',
        '../../src/compiler/basic-block-instrumentor.cc',
        '../../src/compiler/basic-block-instrumentor.h',
        '../../src/compiler/branch-elimination.cc',
        '../../src/compiler/branch-elimination.h',
        '../../src/compiler/bytecode-branch-analysis.cc',
        '../../src/compiler/bytecode-branch-analysis.h',
        '../../src/compiler/bytecode-graph-builder.cc',
        '../../src/compiler/bytecode-graph-builder.h',
        '../../src/compiler/change-lowering.cc',
        '../../src/compiler/change-lowering.h',
        '../../src/compiler/c-linkage.cc',
        '../../src/compiler/coalesced-live-ranges.cc',
        '../../src/compiler/coalesced-live-ranges.h',
        '../../src/compiler/code-generator-impl.h',
        '../../src/compiler/code-generator.cc',
        '../../src/compiler/code-generator.h',
        '../../src/compiler/code-stub-assembler.cc',
        '../../src/compiler/code-stub-assembler.h',
        '../../src/compiler/common-node-cache.cc',
        '../../src/compiler/common-node-cache.h',
        '../../src/compiler/common-operator-reducer.cc',
        '../../src/compiler/common-operator-reducer.h',
        '../../src/compiler/common-operator.cc',
        '../../src/compiler/common-operator.h',
        '../../src/compiler/control-builders.cc',
        '../../src/compiler/control-builders.h',
        '../../src/compiler/control-equivalence.cc',
        '../../src/compiler/control-equivalence.h',
        '../../src/compiler/control-flow-optimizer.cc',
        '../../src/compiler/control-flow-optimizer.h',
        '../../src/compiler/dead-code-elimination.cc',
        '../../src/compiler/dead-code-elimination.h',
        '../../src/compiler/diamond.h',
        '../../src/compiler/escape-analysis.cc',
        '../../src/compiler/escape-analysis.h',
        "../../src/compiler/escape-analysis-reducer.cc",
        "../../src/compiler/escape-analysis-reducer.h",
        '../../src/compiler/fast-accessor-assembler.cc',
        '../../src/compiler/fast-accessor-assembler.h',
        '../../src/compiler/frame.cc',
        '../../src/compiler/frame.h',
        '../../src/compiler/frame-elider.cc',
        '../../src/compiler/frame-elider.h',
        "../../src/compiler/frame-states.cc",
        "../../src/compiler/frame-states.h",
        '../../src/compiler/gap-resolver.cc',
        '../../src/compiler/gap-resolver.h',
        '../../src/compiler/graph-reducer.cc',
        '../../src/compiler/graph-reducer.h',
        '../../src/compiler/graph-replay.cc',
        '../../src/compiler/graph-replay.h',
        '../../src/compiler/graph-trimmer.cc',
        '../../src/compiler/graph-trimmer.h',
        '../../src/compiler/graph-visualizer.cc',
        '../../src/compiler/graph-visualizer.h',
        '../../src/compiler/graph.cc',
        '../../src/compiler/graph.h',
        '../../src/compiler/greedy-allocator.cc',
        '../../src/compiler/greedy-allocator.h',
        '../../src/compiler/instruction-codes.h',
        '../../src/compiler/instruction-selector-impl.h',
        '../../src/compiler/instruction-selector.cc',
        '../../src/compiler/instruction-selector.h',
        '../../src/compiler/instruction-scheduler.cc',
        '../../src/compiler/instruction-scheduler.h',
        '../../src/compiler/instruction.cc',
        '../../src/compiler/instruction.h',
        '../../src/compiler/int64-lowering.cc',
        '../../src/compiler/int64-lowering.h',
        '../../src/compiler/js-builtin-reducer.cc',
        '../../src/compiler/js-builtin-reducer.h',
        '../../src/compiler/js-call-reducer.cc',
        '../../src/compiler/js-call-reducer.h',
        '../../src/compiler/js-context-specialization.cc',
        '../../src/compiler/js-context-specialization.h',
        '../../src/compiler/js-create-lowering.cc',
        '../../src/compiler/js-create-lowering.h',
        '../../src/compiler/js-frame-specialization.cc',
        '../../src/compiler/js-frame-specialization.h',
        '../../src/compiler/js-generic-lowering.cc',
        '../../src/compiler/js-generic-lowering.h',
        '../../src/compiler/js-global-object-specialization.cc',
        '../../src/compiler/js-global-object-specialization.h',
        '../../src/compiler/js-graph.cc',
        '../../src/compiler/js-graph.h',
        '../../src/compiler/js-inlining.cc',
        '../../src/compiler/js-inlining.h',
        '../../src/compiler/js-inlining-heuristic.cc',
        '../../src/compiler/js-inlining-heuristic.h',
        '../../src/compiler/js-intrinsic-lowering.cc',
        '../../src/compiler/js-intrinsic-lowering.h',
        '../../src/compiler/js-native-context-specialization.cc',
        '../../src/compiler/js-native-context-specialization.h',
        '../../src/compiler/js-operator.cc',
        '../../src/compiler/js-operator.h',
        '../../src/compiler/js-typed-lowering.cc',
        '../../src/compiler/js-typed-lowering.h',
        '../../src/compiler/jump-threading.cc',
        '../../src/compiler/jump-threading.h',
        '../../src/compiler/linkage.cc',
        '../../src/compiler/linkage.h',
        '../../src/compiler/liveness-analyzer.cc',
        '../../src/compiler/liveness-analyzer.h',
        '../../src/compiler/live-range-separator.cc',
        '../../src/compiler/live-range-separator.h',
        '../../src/compiler/load-elimination.cc',
        '../../src/compiler/load-elimination.h',
        '../../src/compiler/loop-analysis.cc',
        '../../src/compiler/loop-analysis.h',
        '../../src/compiler/loop-peeling.cc',
        '../../src/compiler/loop-peeling.h',
        '../../src/compiler/machine-operator-reducer.cc',
        '../../src/compiler/machine-operator-reducer.h',
        '../../src/compiler/machine-operator.cc',
        '../../src/compiler/machine-operator.h',
        '../../src/compiler/move-optimizer.cc',
        '../../src/compiler/move-optimizer.h',
        '../../src/compiler/node-aux-data.h',
        '../../src/compiler/node-cache.cc',
        '../../src/compiler/node-cache.h',
        '../../src/compiler/node-marker.cc',
        '../../src/compiler/node-marker.h',
        '../../src/compiler/node-matchers.cc',
        '../../src/compiler/node-matchers.h',
        '../../src/compiler/node-properties.cc',
        '../../src/compiler/node-properties.h',
        '../../src/compiler/node.cc',
        '../../src/compiler/node.h',
        '../../src/compiler/opcodes.cc',
        '../../src/compiler/opcodes.h',
        '../../src/compiler/operator-properties.cc',
        '../../src/compiler/operator-properties.h',
        '../../src/compiler/operator.cc',
        '../../src/compiler/operator.h',
        '../../src/compiler/osr.cc',
        '../../src/compiler/osr.h',
        '../../src/compiler/pipeline.cc',
        '../../src/compiler/pipeline.h',
        '../../src/compiler/pipeline-statistics.cc',
        '../../src/compiler/pipeline-statistics.h',
        '../../src/compiler/raw-machine-assembler.cc',
        '../../src/compiler/raw-machine-assembler.h',
        '../../src/compiler/register-allocator.cc',
        '../../src/compiler/register-allocator.h',
        '../../src/compiler/register-allocator-verifier.cc',
        '../../src/compiler/register-allocator-verifier.h',
        '../../src/compiler/representation-change.cc',
        '../../src/compiler/representation-change.h',
        '../../src/compiler/schedule.cc',
        '../../src/compiler/schedule.h',
        '../../src/compiler/scheduler.cc',
        '../../src/compiler/scheduler.h',
        '../../src/compiler/select-lowering.cc',
        '../../src/compiler/select-lowering.h',
        '../../src/compiler/simplified-lowering.cc',
        '../../src/compiler/simplified-lowering.h',
        '../../src/compiler/simplified-operator-reducer.cc',
        '../../src/compiler/simplified-operator-reducer.h',
        '../../src/compiler/simplified-operator.cc',
        '../../src/compiler/simplified-operator.h',
        '../../src/compiler/source-position.cc',
        '../../src/compiler/source-position.h',
        '../../src/compiler/state-values-utils.cc',
        '../../src/compiler/state-values-utils.h',
        '../../src/compiler/tail-call-optimization.cc',
        '../../src/compiler/tail-call-optimization.h',
        '../../src/compiler/type-hint-analyzer.cc',
        '../../src/compiler/type-hint-analyzer.h',
        '../../src/compiler/type-hints.cc',
        '../../src/compiler/type-hints.h',
        '../../src/compiler/typer.cc',
        '../../src/compiler/typer.h',
        '../../src/compiler/value-numbering-reducer.cc',
        '../../src/compiler/value-numbering-reducer.h',
        '../../src/compiler/verifier.cc',
        '../../src/compiler/verifier.h',
        '../../src/compiler/wasm-compiler.cc',
        '../../src/compiler/wasm-compiler.h',
        '../../src/compiler/wasm-linkage.cc',
        '../../src/compiler/zone-pool.cc',
        '../../src/compiler/zone-pool.h',
        '../../src/compiler.cc',
        '../../src/compiler.h',
        '../../src/context-measure.cc',
        '../../src/context-measure.h',
        '../../src/contexts-inl.h',
        '../../src/contexts.cc',
        '../../src/contexts.h',
        '../../src/conversions-inl.h',
        '../../src/conversions.cc',
        '../../src/conversions.h',
        '../../src/counters.cc',
        '../../src/counters.h',
        '../../src/crankshaft/hydrogen-alias-analysis.h',
        '../../src/crankshaft/hydrogen-bce.cc',
        '../../src/crankshaft/hydrogen-bce.h',
        '../../src/crankshaft/hydrogen-bch.cc',
        '../../src/crankshaft/hydrogen-bch.h',
        '../../src/crankshaft/hydrogen-canonicalize.cc',
        '../../src/crankshaft/hydrogen-canonicalize.h',
        '../../src/crankshaft/hydrogen-check-elimination.cc',
        '../../src/crankshaft/hydrogen-check-elimination.h',
        '../../src/crankshaft/hydrogen-dce.cc',
        '../../src/crankshaft/hydrogen-dce.h',
        '../../src/crankshaft/hydrogen-dehoist.cc',
        '../../src/crankshaft/hydrogen-dehoist.h',
        '../../src/crankshaft/hydrogen-environment-liveness.cc',
        '../../src/crankshaft/hydrogen-environment-liveness.h',
        '../../src/crankshaft/hydrogen-escape-analysis.cc',
        '../../src/crankshaft/hydrogen-escape-analysis.h',
        '../../src/crankshaft/hydrogen-flow-engine.h',
        '../../src/crankshaft/hydrogen-gvn.cc',
        '../../src/crankshaft/hydrogen-gvn.h',
        '../../src/crankshaft/hydrogen-infer-representation.cc',
        '../../src/crankshaft/hydrogen-infer-representation.h',
        '../../src/crankshaft/hydrogen-infer-types.cc',
        '../../src/crankshaft/hydrogen-infer-types.h',
        '../../src/crankshaft/hydrogen-instructions.cc',
        '../../src/crankshaft/hydrogen-instructions.h',
        '../../src/crankshaft/hydrogen-load-elimination.cc',
        '../../src/crankshaft/hydrogen-load-elimination.h',
        '../../src/crankshaft/hydrogen-mark-deoptimize.cc',
        '../../src/crankshaft/hydrogen-mark-deoptimize.h',
        '../../src/crankshaft/hydrogen-mark-unreachable.cc',
        '../../src/crankshaft/hydrogen-mark-unreachable.h',
        '../../src/crankshaft/hydrogen-osr.cc',
        '../../src/crankshaft/hydrogen-osr.h',
        '../../src/crankshaft/hydrogen-range-analysis.cc',
        '../../src/crankshaft/hydrogen-range-analysis.h',
        '../../src/crankshaft/hydrogen-redundant-phi.cc',
        '../../src/crankshaft/hydrogen-redundant-phi.h',
        '../../src/crankshaft/hydrogen-removable-simulates.cc',
        '../../src/crankshaft/hydrogen-removable-simulates.h',
        '../../src/crankshaft/hydrogen-representation-changes.cc',
        '../../src/crankshaft/hydrogen-representation-changes.h',
        '../../src/crankshaft/hydrogen-sce.cc',
        '../../src/crankshaft/hydrogen-sce.h',
        '../../src/crankshaft/hydrogen-store-elimination.cc',
        '../../src/crankshaft/hydrogen-store-elimination.h',
        '../../src/crankshaft/hydrogen-types.cc',
        '../../src/crankshaft/hydrogen-types.h',
        '../../src/crankshaft/hydrogen-uint32-analysis.cc',
        '../../src/crankshaft/hydrogen-uint32-analysis.h',
        '../../src/crankshaft/hydrogen.cc',
        '../../src/crankshaft/hydrogen.h',
        '../../src/crankshaft/lithium-allocator-inl.h',
        '../../src/crankshaft/lithium-allocator.cc',
        '../../src/crankshaft/lithium-allocator.h',
        '../../src/crankshaft/lithium-codegen.cc',
        '../../src/crankshaft/lithium-codegen.h',
        '../../src/crankshaft/lithium.cc',
        '../../src/crankshaft/lithium.h',
        '../../src/crankshaft/lithium-inl.h',
        '../../src/crankshaft/typing.cc',
        '../../src/crankshaft/typing.h',
        '../../src/crankshaft/unique.h',
        '../../src/date.cc',
        '../../src/date.h',
        '../../src/dateparser-inl.h',
        '../../src/dateparser.cc',
        '../../src/dateparser.h',
        '../../src/debug/debug-evaluate.cc',
        '../../src/debug/debug-evaluate.h',
        '../../src/debug/debug-frames.cc',
        '../../src/debug/debug-frames.h',
        '../../src/debug/debug-scopes.cc',
        '../../src/debug/debug-scopes.h',
        '../../src/debug/debug.cc',
        '../../src/debug/debug.h',
        '../../src/debug/liveedit.cc',
        '../../src/debug/liveedit.h',
        '../../src/deoptimizer.cc',
        '../../src/deoptimizer.h',
        '../../src/disasm.h',
        '../../src/disassembler.cc',
        '../../src/disassembler.h',
        '../../src/diy-fp.cc',
        '../../src/diy-fp.h',
        '../../src/double.h',
        '../../src/dtoa.cc',
        '../../src/dtoa.h',
        '../../src/effects.h',
        '../../src/elements-kind.cc',
        '../../src/elements-kind.h',
        '../../src/elements.cc',
        '../../src/elements.h',
        '../../src/execution.cc',
        '../../src/execution.h',
        '../../src/extensions/externalize-string-extension.cc',
        '../../src/extensions/externalize-string-extension.h',
        '../../src/extensions/free-buffer-extension.cc',
        '../../src/extensions/free-buffer-extension.h',
        '../../src/extensions/gc-extension.cc',
        '../../src/extensions/gc-extension.h',
        '../../src/extensions/statistics-extension.cc',
        '../../src/extensions/statistics-extension.h',
        '../../src/extensions/trigger-failure-extension.cc',
        '../../src/extensions/trigger-failure-extension.h',
        '../../src/factory.cc',
        '../../src/factory.h',
        '../../src/fast-dtoa.cc',
        '../../src/fast-dtoa.h',
        '../../src/field-index.h',
        '../../src/field-index-inl.h',
        '../../src/field-type.cc',
        '../../src/field-type.h',
        '../../src/fixed-dtoa.cc',
        '../../src/fixed-dtoa.h',
        '../../src/flag-definitions.h',
        '../../src/flags.cc',
        '../../src/flags.h',
        '../../src/frames-inl.h',
        '../../src/frames.cc',
        '../../src/frames.h',
        '../../src/full-codegen/full-codegen.cc',
        '../../src/full-codegen/full-codegen.h',
        '../../src/futex-emulation.cc',
        '../../src/futex-emulation.h',
        '../../src/gdb-jit.cc',
        '../../src/gdb-jit.h',
        '../../src/global-handles.cc',
        '../../src/global-handles.h',
        '../../src/globals.h',
        '../../src/handles-inl.h',
        '../../src/handles.cc',
        '../../src/handles.h',
        '../../src/hashmap.h',
        '../../src/heap-symbols.h',
        '../../src/heap/array-buffer-tracker.cc',
        '../../src/heap/array-buffer-tracker.h',
        '../../src/heap/memory-reducer.cc',
        '../../src/heap/memory-reducer.h',
        '../../src/heap/gc-idle-time-handler.cc',
        '../../src/heap/gc-idle-time-handler.h',
        '../../src/heap/gc-tracer.cc',
        '../../src/heap/gc-tracer.h',
        '../../src/heap/heap-inl.h',
        '../../src/heap/heap.cc',
        '../../src/heap/heap.h',
        '../../src/heap/incremental-marking-inl.h',
        '../../src/heap/incremental-marking-job.cc',
        '../../src/heap/incremental-marking-job.h',
        '../../src/heap/incremental-marking.cc',
        '../../src/heap/incremental-marking.h',
        '../../src/heap/mark-compact-inl.h',
        '../../src/heap/mark-compact.cc',
        '../../src/heap/mark-compact.h',
        '../../src/heap/object-stats.cc',
        '../../src/heap/object-stats.h',
        '../../src/heap/objects-visiting-inl.h',
        '../../src/heap/objects-visiting.cc',
        '../../src/heap/objects-visiting.h',
        '../../src/heap/remembered-set.cc',
        '../../src/heap/remembered-set.h',
        '../../src/heap/scavenge-job.h',
        '../../src/heap/scavenge-job.cc',
        '../../src/heap/scavenger-inl.h',
        '../../src/heap/scavenger.cc',
        '../../src/heap/scavenger.h',
        '../../src/heap/slot-set.h',
        '../../src/heap/slots-buffer.cc',
        '../../src/heap/slots-buffer.h',
        '../../src/heap/spaces-inl.h',
        '../../src/heap/spaces.cc',
        '../../src/heap/spaces.h',
        '../../src/heap/store-buffer-inl.h',
        '../../src/heap/store-buffer.cc',
        '../../src/heap/store-buffer.h',
        '../../src/i18n.cc',
        '../../src/i18n.h',
        '../../src/icu_util.cc',
        '../../src/icu_util.h',
        '../../src/ic/access-compiler.cc',
        '../../src/ic/access-compiler.h',
        '../../src/ic/call-optimization.cc',
        '../../src/ic/call-optimization.h',
        '../../src/ic/handler-compiler.cc',
        '../../src/ic/handler-compiler.h',
        '../../src/ic/ic-inl.h',
        '../../src/ic/ic-state.cc',
        '../../src/ic/ic-state.h',
        '../../src/ic/ic.cc',
        '../../src/ic/ic.h',
        '../../src/ic/ic-compiler.cc',
        '../../src/ic/ic-compiler.h',
        '../../src/identity-map.cc',
        '../../src/identity-map.h',
        '../../src/interface-descriptors.cc',
        '../../src/interface-descriptors.h',
        '../../src/interpreter/bytecodes.cc',
        '../../src/interpreter/bytecodes.h',
        '../../src/interpreter/bytecode-array-builder.cc',
        '../../src/interpreter/bytecode-array-builder.h',
        '../../src/interpreter/bytecode-array-iterator.cc',
        '../../src/interpreter/bytecode-array-iterator.h',
        '../../src/interpreter/bytecode-register-allocator.cc',
        '../../src/interpreter/bytecode-register-allocator.h',
        '../../src/interpreter/bytecode-generator.cc',
        '../../src/interpreter/bytecode-generator.h',
        '../../src/interpreter/bytecode-traits.h',
        '../../src/interpreter/constant-array-builder.cc',
        '../../src/interpreter/constant-array-builder.h',
        '../../src/interpreter/control-flow-builders.cc',
        '../../src/interpreter/control-flow-builders.h',
        '../../src/interpreter/handler-table-builder.cc',
        '../../src/interpreter/handler-table-builder.h',
        '../../src/interpreter/interpreter.cc',
        '../../src/interpreter/interpreter.h',
        '../../src/interpreter/interpreter-assembler.cc',
        '../../src/interpreter/interpreter-assembler.h',
        '../../src/interpreter/register-translator.cc',
        '../../src/interpreter/register-translator.h',
        '../../src/interpreter/source-position-table.cc',
        '../../src/interpreter/source-position-table.h',
        '../../src/isolate-inl.h',
        '../../src/isolate.cc',
        '../../src/isolate.h',
        '../../src/json-parser.h',
        '../../src/json-stringifier.h',
        '../../src/key-accumulator.h',
        '../../src/key-accumulator.cc',
        '../../src/layout-descriptor-inl.h',
        '../../src/layout-descriptor.cc',
        '../../src/layout-descriptor.h',
        '../../src/list-inl.h',
        '../../src/list.h',
        '../../src/locked-queue-inl.h',
        '../../src/locked-queue.h',
        '../../src/log-inl.h',
        '../../src/log-utils.cc',
        '../../src/log-utils.h',
        '../../src/log.cc',
        '../../src/log.h',
        '../../src/lookup.cc',
        '../../src/lookup.h',
        '../../src/macro-assembler.h',
        '../../src/machine-type.cc',
        '../../src/machine-type.h',
        '../../src/messages.cc',
        '../../src/messages.h',
        '../../src/msan.h',
        '../../src/objects-body-descriptors-inl.h',
        '../../src/objects-body-descriptors.h',
        '../../src/objects-debug.cc',
        '../../src/objects-inl.h',
        '../../src/objects-printer.cc',
        '../../src/objects.cc',
        '../../src/objects.h',
        '../../src/optimizing-compile-dispatcher.cc',
        '../../src/optimizing-compile-dispatcher.h',
        '../../src/ostreams.cc',
        '../../src/ostreams.h',
        '../../src/parsing/expression-classifier.h',
        '../../src/parsing/func-name-inferrer.cc',
        '../../src/parsing/func-name-inferrer.h',
        '../../src/parsing/parameter-initializer-rewriter.cc',
        '../../src/parsing/parameter-initializer-rewriter.h',
        '../../src/parsing/parser-base.h',
        '../../src/parsing/parser.cc',
        '../../src/parsing/parser.h',
        '../../src/parsing/pattern-rewriter.cc',
        '../../src/parsing/preparse-data-format.h',
        '../../src/parsing/preparse-data.cc',
        '../../src/parsing/preparse-data.h',
        '../../src/parsing/preparser.cc',
        '../../src/parsing/preparser.h',
        '../../src/parsing/rewriter.cc',
        '../../src/parsing/rewriter.h',
        '../../src/parsing/scanner-character-streams.cc',
        '../../src/parsing/scanner-character-streams.h',
        '../../src/parsing/scanner.cc',
        '../../src/parsing/scanner.h',
        '../../src/parsing/token.cc',
        '../../src/parsing/token.h',
        '../../src/pending-compilation-error-handler.cc',
        '../../src/pending-compilation-error-handler.h',
        '../../src/profiler/allocation-tracker.cc',
        '../../src/profiler/allocation-tracker.h',
        '../../src/profiler/circular-queue-inl.h',
        '../../src/profiler/circular-queue.h',
        '../../src/profiler/cpu-profiler-inl.h',
        '../../src/profiler/cpu-profiler.cc',
        '../../src/profiler/cpu-profiler.h',
        '../../src/profiler/heap-profiler.cc',
        '../../src/profiler/heap-profiler.h',
        '../../src/profiler/heap-snapshot-generator-inl.h',
        '../../src/profiler/heap-snapshot-generator.cc',
        '../../src/profiler/heap-snapshot-generator.h',
        '../../src/profiler/profile-generator-inl.h',
        '../../src/profiler/profile-generator.cc',
        '../../src/profiler/profile-generator.h',
        '../../src/profiler/sampler.cc',
        '../../src/profiler/sampler.h',
        '../../src/profiler/sampling-heap-profiler.cc',
        '../../src/profiler/sampling-heap-profiler.h',
        '../../src/profiler/strings-storage.cc',
        '../../src/profiler/strings-storage.h',
        '../../src/profiler/unbound-queue-inl.h',
        '../../src/profiler/unbound-queue.h',
        '../../src/property-descriptor.cc',
        '../../src/property-descriptor.h',
        '../../src/property-details.h',
        '../../src/property.cc',
        '../../src/property.h',
        '../../src/prototype.h',
        '../../src/regexp/bytecodes-irregexp.h',
        '../../src/regexp/interpreter-irregexp.cc',
        '../../src/regexp/interpreter-irregexp.h',
        '../../src/regexp/jsregexp-inl.h',
        '../../src/regexp/jsregexp.cc',
        '../../src/regexp/jsregexp.h',
        '../../src/regexp/regexp-ast.cc',
        '../../src/regexp/regexp-ast.h',
        '../../src/regexp/regexp-macro-assembler-irregexp-inl.h',
        '../../src/regexp/regexp-macro-assembler-irregexp.cc',
        '../../src/regexp/regexp-macro-assembler-irregexp.h',
        '../../src/regexp/regexp-macro-assembler-tracer.cc',
        '../../src/regexp/regexp-macro-assembler-tracer.h',
        '../../src/regexp/regexp-macro-assembler.cc',
        '../../src/regexp/regexp-macro-assembler.h',
        '../../src/regexp/regexp-parser.cc',
        '../../src/regexp/regexp-parser.h',
        '../../src/regexp/regexp-stack.cc',
        '../../src/regexp/regexp-stack.h',
        '../../src/register-configuration.cc',
        '../../src/register-configuration.h',
        '../../src/runtime-profiler.cc',
        '../../src/runtime-profiler.h',
        '../../src/runtime/runtime-array.cc',
        '../../src/runtime/runtime-atomics.cc',
        '../../src/runtime/runtime-classes.cc',
        '../../src/runtime/runtime-collections.cc',
        '../../src/runtime/runtime-compiler.cc',
        '../../src/runtime/runtime-date.cc',
        '../../src/runtime/runtime-debug.cc',
        '../../src/runtime/runtime-forin.cc',
        '../../src/runtime/runtime-function.cc',
        '../../src/runtime/runtime-futex.cc',
        '../../src/runtime/runtime-generator.cc',
        '../../src/runtime/runtime-i18n.cc',
        '../../src/runtime/runtime-internal.cc',
        '../../src/runtime/runtime-interpreter.cc',
        '../../src/runtime/runtime-json.cc',
        '../../src/runtime/runtime-literals.cc',
        '../../src/runtime/runtime-liveedit.cc',
        '../../src/runtime/runtime-maths.cc',
        '../../src/runtime/runtime-numbers.cc',
        '../../src/runtime/runtime-object.cc',
        '../../src/runtime/runtime-observe.cc',
        '../../src/runtime/runtime-operators.cc',
        '../../src/runtime/runtime-proxy.cc',
        '../../src/runtime/runtime-regexp.cc',
        '../../src/runtime/runtime-scopes.cc',
        '../../src/runtime/runtime-simd.cc',
        '../../src/runtime/runtime-strings.cc',
        '../../src/runtime/runtime-symbol.cc',
        '../../src/runtime/runtime-test.cc',
        '../../src/runtime/runtime-typedarray.cc',
        '../../src/runtime/runtime-uri.cc',
        '../../src/runtime/runtime-utils.h',
        '../../src/runtime/runtime.cc',
        '../../src/runtime/runtime.h',
        '../../src/safepoint-table.cc',
        '../../src/safepoint-table.h',
        '../../src/signature.h',
        '../../src/simulator.h',
        '../../src/small-pointer-list.h',
        '../../src/snapshot/natives.h',
        '../../src/snapshot/natives-common.cc',
        '../../src/snapshot/serialize.cc',
        '../../src/snapshot/serialize.h',
        '../../src/snapshot/snapshot.h',
        '../../src/snapshot/snapshot-common.cc',
        '../../src/snapshot/snapshot-source-sink.cc',
        '../../src/snapshot/snapshot-source-sink.h',
        '../../src/source-position.h',
        '../../src/splay-tree.h',
        '../../src/splay-tree-inl.h',
        '../../src/startup-data-util.cc',
        '../../src/startup-data-util.h',
        '../../src/string-builder.cc',
        '../../src/string-builder.h',
        '../../src/string-search.h',
        '../../src/string-stream.cc',
        '../../src/string-stream.h',
        '../../src/strtod.cc',
        '../../src/strtod.h',
        '../../src/ic/stub-cache.cc',
        '../../src/ic/stub-cache.h',
        '../../src/tracing/trace-event.cc',
        '../../src/tracing/trace-event.h',
        '../../src/transitions-inl.h',
        '../../src/transitions.cc',
        '../../src/transitions.h',
        '../../src/type-cache.cc',
        '../../src/type-cache.h',
        '../../src/type-feedback-vector-inl.h',
        '../../src/type-feedback-vector.cc',
        '../../src/type-feedback-vector.h',
        '../../src/type-info.cc',
        '../../src/type-info.h',
        '../../src/types.cc',
        '../../src/types.h',
        '../../src/typing-asm.cc',
        '../../src/typing-asm.h',
        '../../src/typing-reset.cc',
        '../../src/typing-reset.h',
        '../../src/unicode-inl.h',
        '../../src/unicode.cc',
        '../../src/unicode.h',
        '../../src/unicode-cache-inl.h',
        '../../src/unicode-cache.h',
        '../../src/unicode-decoder.cc',
        '../../src/unicode-decoder.h',
        '../../src/utils-inl.h',
        '../../src/utils.cc',
        '../../src/utils.h',
        '../../src/v8.cc',
        '../../src/v8.h',
        '../../src/v8memory.h',
        '../../src/v8threads.cc',
        '../../src/v8threads.h',
        '../../src/vector.h',
        '../../src/version.cc',
        '../../src/version.h',
        '../../src/vm-state-inl.h',
        '../../src/vm-state.h',
        '../../src/wasm/asm-wasm-builder.cc',
        '../../src/wasm/asm-wasm-builder.h',
        '../../src/wasm/ast-decoder.cc',
        '../../src/wasm/ast-decoder.h',
        '../../src/wasm/decoder.h',
        '../../src/wasm/encoder.cc',
        '../../src/wasm/encoder.h',
        '../../src/wasm/module-decoder.cc',
        '../../src/wasm/module-decoder.h',
        '../../src/wasm/wasm-js.cc',
        '../../src/wasm/wasm-js.h',
        '../../src/wasm/wasm-macro-gen.h',
        '../../src/wasm/wasm-module.cc',
        '../../src/wasm/wasm-module.h',
        '../../src/wasm/wasm-opcodes.cc',
        '../../src/wasm/wasm-opcodes.h',
        '../../src/wasm/wasm-result.cc',
        '../../src/wasm/wasm-result.h',
        '../../src/zone.cc',
        '../../src/zone.h',
        '../../src/zone-allocator.h',
        '../../src/zone-containers.h',
        '../../src/third_party/fdlibm/fdlibm.cc',
        '../../src/third_party/fdlibm/fdlibm.h',
      ],
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
        }, {
          'toolsets': ['target'],
        }],
        ['v8_target_arch=="arm"', {
          'sources': [  ### gcmole(arch:arm) ###
            '../../src/arm/assembler-arm-inl.h',
            '../../src/arm/assembler-arm.cc',
            '../../src/arm/assembler-arm.h',
            '../../src/arm/builtins-arm.cc',
            '../../src/arm/code-stubs-arm.cc',
            '../../src/arm/code-stubs-arm.h',
            '../../src/arm/codegen-arm.cc',
            '../../src/arm/codegen-arm.h',
            '../../src/arm/constants-arm.h',
            '../../src/arm/constants-arm.cc',
            '../../src/arm/cpu-arm.cc',
            '../../src/arm/deoptimizer-arm.cc',
            '../../src/arm/disasm-arm.cc',
            '../../src/arm/frames-arm.cc',
            '../../src/arm/frames-arm.h',
            '../../src/arm/interface-descriptors-arm.cc',
            '../../src/arm/interface-descriptors-arm.h',
            '../../src/arm/macro-assembler-arm.cc',
            '../../src/arm/macro-assembler-arm.h',
            '../../src/arm/simulator-arm.cc',
            '../../src/arm/simulator-arm.h',
            '../../src/compiler/arm/code-generator-arm.cc',
            '../../src/compiler/arm/instruction-codes-arm.h',
            '../../src/compiler/arm/instruction-scheduler-arm.cc',
            '../../src/compiler/arm/instruction-selector-arm.cc',
            '../../src/crankshaft/arm/lithium-arm.cc',
            '../../src/crankshaft/arm/lithium-arm.h',
            '../../src/crankshaft/arm/lithium-codegen-arm.cc',
            '../../src/crankshaft/arm/lithium-codegen-arm.h',
            '../../src/crankshaft/arm/lithium-gap-resolver-arm.cc',
            '../../src/crankshaft/arm/lithium-gap-resolver-arm.h',
            '../../src/debug/arm/debug-arm.cc',
            '../../src/full-codegen/arm/full-codegen-arm.cc',
            '../../src/ic/arm/access-compiler-arm.cc',
            '../../src/ic/arm/handler-compiler-arm.cc',
            '../../src/ic/arm/ic-arm.cc',
            '../../src/ic/arm/ic-compiler-arm.cc',
            '../../src/ic/arm/stub-cache-arm.cc',
            '../../src/regexp/arm/regexp-macro-assembler-arm.cc',
            '../../src/regexp/arm/regexp-macro-assembler-arm.h',
          ],
        }],
        ['v8_target_arch=="arm64"', {
          'sources': [  ### gcmole(arch:arm64) ###
            '../../src/arm64/assembler-arm64.cc',
            '../../src/arm64/assembler-arm64.h',
            '../../src/arm64/assembler-arm64-inl.h',
            '../../src/arm64/builtins-arm64.cc',
            '../../src/arm64/codegen-arm64.cc',
            '../../src/arm64/codegen-arm64.h',
            '../../src/arm64/code-stubs-arm64.cc',
            '../../src/arm64/code-stubs-arm64.h',
            '../../src/arm64/constants-arm64.h',
            '../../src/arm64/cpu-arm64.cc',
            '../../src/arm64/decoder-arm64.cc',
            '../../src/arm64/decoder-arm64.h',
            '../../src/arm64/decoder-arm64-inl.h',
            '../../src/arm64/deoptimizer-arm64.cc',
            '../../src/arm64/disasm-arm64.cc',
            '../../src/arm64/disasm-arm64.h',
            '../../src/arm64/frames-arm64.cc',
            '../../src/arm64/frames-arm64.h',
            '../../src/arm64/instructions-arm64.cc',
            '../../src/arm64/instructions-arm64.h',
            '../../src/arm64/instrument-arm64.cc',
            '../../src/arm64/instrument-arm64.h',
            '../../src/arm64/interface-descriptors-arm64.cc',
            '../../src/arm64/interface-descriptors-arm64.h',
            '../../src/arm64/macro-assembler-arm64.cc',
            '../../src/arm64/macro-assembler-arm64.h',
            '../../src/arm64/macro-assembler-arm64-inl.h',
            '../../src/arm64/simulator-arm64.cc',
            '../../src/arm64/simulator-arm64.h',
            '../../src/arm64/utils-arm64.cc',
            '../../src/arm64/utils-arm64.h',
            '../../src/compiler/arm64/code-generator-arm64.cc',
            '../../src/compiler/arm64/instruction-codes-arm64.h',
            '../../src/compiler/arm64/instruction-scheduler-arm64.cc',
            '../../src/compiler/arm64/instruction-selector-arm64.cc',
            '../../src/crankshaft/arm64/delayed-masm-arm64.cc',
            '../../src/crankshaft/arm64/delayed-masm-arm64.h',
            '../../src/crankshaft/arm64/delayed-masm-arm64-inl.h',
            '../../src/crankshaft/arm64/lithium-arm64.cc',
            '../../src/crankshaft/arm64/lithium-arm64.h',
            '../../src/crankshaft/arm64/lithium-codegen-arm64.cc',
            '../../src/crankshaft/arm64/lithium-codegen-arm64.h',
            '../../src/crankshaft/arm64/lithium-gap-resolver-arm64.cc',
            '../../src/crankshaft/arm64/lithium-gap-resolver-arm64.h',
            '../../src/debug/arm64/debug-arm64.cc',
            '../../src/full-codegen/arm64/full-codegen-arm64.cc',
            '../../src/ic/arm64/access-compiler-arm64.cc',
            '../../src/ic/arm64/handler-compiler-arm64.cc',
            '../../src/ic/arm64/ic-arm64.cc',
            '../../src/ic/arm64/ic-compiler-arm64.cc',
            '../../src/ic/arm64/stub-cache-arm64.cc',
            '../../src/regexp/arm64/regexp-macro-assembler-arm64.cc',
            '../../src/regexp/arm64/regexp-macro-assembler-arm64.h',
          ],
        }],
        ['v8_target_arch=="ia32"', {
          'sources': [  ### gcmole(arch:ia32) ###
            '../../src/ia32/assembler-ia32-inl.h',
            '../../src/ia32/assembler-ia32.cc',
            '../../src/ia32/assembler-ia32.h',
            '../../src/ia32/builtins-ia32.cc',
            '../../src/ia32/code-stubs-ia32.cc',
            '../../src/ia32/code-stubs-ia32.h',
            '../../src/ia32/codegen-ia32.cc',
            '../../src/ia32/codegen-ia32.h',
            '../../src/ia32/cpu-ia32.cc',
            '../../src/ia32/deoptimizer-ia32.cc',
            '../../src/ia32/disasm-ia32.cc',
            '../../src/ia32/frames-ia32.cc',
            '../../src/ia32/frames-ia32.h',
            '../../src/ia32/interface-descriptors-ia32.cc',
            '../../src/ia32/macro-assembler-ia32.cc',
            '../../src/ia32/macro-assembler-ia32.h',
            '../../src/compiler/ia32/code-generator-ia32.cc',
            '../../src/compiler/ia32/instruction-codes-ia32.h',
            '../../src/compiler/ia32/instruction-scheduler-ia32.cc',
            '../../src/compiler/ia32/instruction-selector-ia32.cc',
            '../../src/crankshaft/ia32/lithium-codegen-ia32.cc',
            '../../src/crankshaft/ia32/lithium-codegen-ia32.h',
            '../../src/crankshaft/ia32/lithium-gap-resolver-ia32.cc',
            '../../src/crankshaft/ia32/lithium-gap-resolver-ia32.h',
            '../../src/crankshaft/ia32/lithium-ia32.cc',
            '../../src/crankshaft/ia32/lithium-ia32.h',
            '../../src/debug/ia32/debug-ia32.cc',
            '../../src/full-codegen/ia32/full-codegen-ia32.cc',
            '../../src/ic/ia32/access-compiler-ia32.cc',
            '../../src/ic/ia32/handler-compiler-ia32.cc',
            '../../src/ic/ia32/ic-ia32.cc',
            '../../src/ic/ia32/ic-compiler-ia32.cc',
            '../../src/ic/ia32/stub-cache-ia32.cc',
            '../../src/regexp/ia32/regexp-macro-assembler-ia32.cc',
            '../../src/regexp/ia32/regexp-macro-assembler-ia32.h',
          ],
        }],
        ['v8_target_arch=="x87"', {
          'sources': [  ### gcmole(arch:x87) ###
            '../../src/x87/assembler-x87-inl.h',
            '../../src/x87/assembler-x87.cc',
            '../../src/x87/assembler-x87.h',
            '../../src/x87/builtins-x87.cc',
            '../../src/x87/code-stubs-x87.cc',
            '../../src/x87/code-stubs-x87.h',
            '../../src/x87/codegen-x87.cc',
            '../../src/x87/codegen-x87.h',
            '../../src/x87/cpu-x87.cc',
            '../../src/x87/deoptimizer-x87.cc',
            '../../src/x87/disasm-x87.cc',
            '../../src/x87/frames-x87.cc',
            '../../src/x87/frames-x87.h',
            '../../src/x87/interface-descriptors-x87.cc',
            '../../src/x87/macro-assembler-x87.cc',
            '../../src/x87/macro-assembler-x87.h',
            '../../src/compiler/x87/code-generator-x87.cc',
            '../../src/compiler/x87/instruction-codes-x87.h',
            '../../src/compiler/x87/instruction-scheduler-x87.cc',
            '../../src/compiler/x87/instruction-selector-x87.cc',
            '../../src/crankshaft/x87/lithium-codegen-x87.cc',
            '../../src/crankshaft/x87/lithium-codegen-x87.h',
            '../../src/crankshaft/x87/lithium-gap-resolver-x87.cc',
            '../../src/crankshaft/x87/lithium-gap-resolver-x87.h',
            '../../src/crankshaft/x87/lithium-x87.cc',
            '../../src/crankshaft/x87/lithium-x87.h',
            '../../src/debug/x87/debug-x87.cc',
            '../../src/full-codegen/x87/full-codegen-x87.cc',
            '../../src/ic/x87/access-compiler-x87.cc',
            '../../src/ic/x87/handler-compiler-x87.cc',
            '../../src/ic/x87/ic-x87.cc',
            '../../src/ic/x87/ic-compiler-x87.cc',
            '../../src/ic/x87/stub-cache-x87.cc',
            '../../src/regexp/x87/regexp-macro-assembler-x87.cc',
            '../../src/regexp/x87/regexp-macro-assembler-x87.h',
          ],
        }],
        ['v8_target_arch=="mips" or v8_target_arch=="mipsel"', {
          'sources': [  ### gcmole(arch:mipsel) ###
            '../../src/mips/assembler-mips.cc',
            '../../src/mips/assembler-mips.h',
            '../../src/mips/assembler-mips-inl.h',
            '../../src/mips/builtins-mips.cc',
            '../../src/mips/codegen-mips.cc',
            '../../src/mips/codegen-mips.h',
            '../../src/mips/code-stubs-mips.cc',
            '../../src/mips/code-stubs-mips.h',
            '../../src/mips/constants-mips.cc',
            '../../src/mips/constants-mips.h',
            '../../src/mips/cpu-mips.cc',
            '../../src/mips/deoptimizer-mips.cc',
            '../../src/mips/disasm-mips.cc',
            '../../src/mips/frames-mips.cc',
            '../../src/mips/frames-mips.h',
            '../../src/mips/interface-descriptors-mips.cc',
            '../../src/mips/macro-assembler-mips.cc',
            '../../src/mips/macro-assembler-mips.h',
            '../../src/mips/simulator-mips.cc',
            '../../src/mips/simulator-mips.h',
            '../../src/compiler/mips/code-generator-mips.cc',
            '../../src/compiler/mips/instruction-codes-mips.h',
            '../../src/compiler/mips/instruction-scheduler-mips.cc',
            '../../src/compiler/mips/instruction-selector-mips.cc',
            '../../src/crankshaft/mips/lithium-codegen-mips.cc',
            '../../src/crankshaft/mips/lithium-codegen-mips.h',
            '../../src/crankshaft/mips/lithium-gap-resolver-mips.cc',
            '../../src/crankshaft/mips/lithium-gap-resolver-mips.h',
            '../../src/crankshaft/mips/lithium-mips.cc',
            '../../src/crankshaft/mips/lithium-mips.h',
            '../../src/full-codegen/mips/full-codegen-mips.cc',
            '../../src/debug/mips/debug-mips.cc',
            '../../src/ic/mips/access-compiler-mips.cc',
            '../../src/ic/mips/handler-compiler-mips.cc',
            '../../src/ic/mips/ic-mips.cc',
            '../../src/ic/mips/ic-compiler-mips.cc',
            '../../src/ic/mips/stub-cache-mips.cc',
            '../../src/regexp/mips/regexp-macro-assembler-mips.cc',
            '../../src/regexp/mips/regexp-macro-assembler-mips.h',
          ],
        }],
        ['v8_target_arch=="mips64" or v8_target_arch=="mips64el"', {
          'sources': [  ### gcmole(arch:mips64el) ###
            '../../src/mips64/assembler-mips64.cc',
            '../../src/mips64/assembler-mips64.h',
            '../../src/mips64/assembler-mips64-inl.h',
            '../../src/mips64/builtins-mips64.cc',
            '../../src/mips64/codegen-mips64.cc',
            '../../src/mips64/codegen-mips64.h',
            '../../src/mips64/code-stubs-mips64.cc',
            '../../src/mips64/code-stubs-mips64.h',
            '../../src/mips64/constants-mips64.cc',
            '../../src/mips64/constants-mips64.h',
            '../../src/mips64/cpu-mips64.cc',
            '../../src/mips64/deoptimizer-mips64.cc',
            '../../src/mips64/disasm-mips64.cc',
            '../../src/mips64/frames-mips64.cc',
            '../../src/mips64/frames-mips64.h',
            '../../src/mips64/interface-descriptors-mips64.cc',
            '../../src/mips64/macro-assembler-mips64.cc',
            '../../src/mips64/macro-assembler-mips64.h',
            '../../src/mips64/simulator-mips64.cc',
            '../../src/mips64/simulator-mips64.h',
            '../../src/compiler/mips64/code-generator-mips64.cc',
            '../../src/compiler/mips64/instruction-codes-mips64.h',
            '../../src/compiler/mips64/instruction-scheduler-mips64.cc',
            '../../src/compiler/mips64/instruction-selector-mips64.cc',
            '../../src/crankshaft/mips64/lithium-codegen-mips64.cc',
            '../../src/crankshaft/mips64/lithium-codegen-mips64.h',
            '../../src/crankshaft/mips64/lithium-gap-resolver-mips64.cc',
            '../../src/crankshaft/mips64/lithium-gap-resolver-mips64.h',
            '../../src/crankshaft/mips64/lithium-mips64.cc',
            '../../src/crankshaft/mips64/lithium-mips64.h',
            '../../src/debug/mips64/debug-mips64.cc',
            '../../src/full-codegen/mips64/full-codegen-mips64.cc',
            '../../src/ic/mips64/access-compiler-mips64.cc',
            '../../src/ic/mips64/handler-compiler-mips64.cc',
            '../../src/ic/mips64/ic-mips64.cc',
            '../../src/ic/mips64/ic-compiler-mips64.cc',
            '../../src/ic/mips64/stub-cache-mips64.cc',
            '../../src/regexp/mips64/regexp-macro-assembler-mips64.cc',
            '../../src/regexp/mips64/regexp-macro-assembler-mips64.h',
          ],
        }],
        ['v8_target_arch=="x64" or v8_target_arch=="x32"', {
          'sources': [  ### gcmole(arch:x64) ###
            '../../src/crankshaft/x64/lithium-codegen-x64.cc',
            '../../src/crankshaft/x64/lithium-codegen-x64.h',
            '../../src/crankshaft/x64/lithium-gap-resolver-x64.cc',
            '../../src/crankshaft/x64/lithium-gap-resolver-x64.h',
            '../../src/crankshaft/x64/lithium-x64.cc',
            '../../src/crankshaft/x64/lithium-x64.h',
            '../../src/x64/assembler-x64-inl.h',
            '../../src/x64/assembler-x64.cc',
            '../../src/x64/assembler-x64.h',
            '../../src/x64/builtins-x64.cc',
            '../../src/x64/code-stubs-x64.cc',
            '../../src/x64/code-stubs-x64.h',
            '../../src/x64/codegen-x64.cc',
            '../../src/x64/codegen-x64.h',
            '../../src/x64/cpu-x64.cc',
            '../../src/x64/deoptimizer-x64.cc',
            '../../src/x64/disasm-x64.cc',
            '../../src/x64/frames-x64.cc',
            '../../src/x64/frames-x64.h',
            '../../src/x64/interface-descriptors-x64.cc',
            '../../src/x64/macro-assembler-x64.cc',
            '../../src/x64/macro-assembler-x64.h',
            '../../src/debug/x64/debug-x64.cc',
            '../../src/full-codegen/x64/full-codegen-x64.cc',
            '../../src/ic/x64/access-compiler-x64.cc',
            '../../src/ic/x64/handler-compiler-x64.cc',
            '../../src/ic/x64/ic-x64.cc',
            '../../src/ic/x64/ic-compiler-x64.cc',
            '../../src/ic/x64/stub-cache-x64.cc',
            '../../src/regexp/x64/regexp-macro-assembler-x64.cc',
            '../../src/regexp/x64/regexp-macro-assembler-x64.h',
          ],
        }],
        ['v8_target_arch=="x64"', {
          'sources': [
            '../../src/compiler/x64/code-generator-x64.cc',
            '../../src/compiler/x64/instruction-codes-x64.h',
            '../../src/compiler/x64/instruction-scheduler-x64.cc',
            '../../src/compiler/x64/instruction-selector-x64.cc',
          ],
        }],
        ['v8_target_arch=="ppc" or v8_target_arch=="ppc64"', {
          'sources': [  ### gcmole(arch:ppc) ###
            '../../src/compiler/ppc/code-generator-ppc.cc',
            '../../src/compiler/ppc/instruction-codes-ppc.h',
            '../../src/compiler/ppc/instruction-scheduler-ppc.cc',
            '../../src/compiler/ppc/instruction-selector-ppc.cc',
            '../../src/crankshaft/ppc/lithium-ppc.cc',
            '../../src/crankshaft/ppc/lithium-ppc.h',
            '../../src/crankshaft/ppc/lithium-codegen-ppc.cc',
            '../../src/crankshaft/ppc/lithium-codegen-ppc.h',
            '../../src/crankshaft/ppc/lithium-gap-resolver-ppc.cc',
            '../../src/crankshaft/ppc/lithium-gap-resolver-ppc.h',
            '../../src/debug/ppc/debug-ppc.cc',
            '../../src/full-codegen/ppc/full-codegen-ppc.cc',
            '../../src/ic/ppc/access-compiler-ppc.cc',
            '../../src/ic/ppc/handler-compiler-ppc.cc',
            '../../src/ic/ppc/ic-ppc.cc',
            '../../src/ic/ppc/ic-compiler-ppc.cc',
            '../../src/ic/ppc/stub-cache-ppc.cc',
            '../../src/ppc/assembler-ppc-inl.h',
            '../../src/ppc/assembler-ppc.cc',
            '../../src/ppc/assembler-ppc.h',
            '../../src/ppc/builtins-ppc.cc',
            '../../src/ppc/code-stubs-ppc.cc',
            '../../src/ppc/code-stubs-ppc.h',
            '../../src/ppc/codegen-ppc.cc',
            '../../src/ppc/codegen-ppc.h',
            '../../src/ppc/constants-ppc.h',
            '../../src/ppc/constants-ppc.cc',
            '../../src/ppc/cpu-ppc.cc',
            '../../src/ppc/deoptimizer-ppc.cc',
            '../../src/ppc/disasm-ppc.cc',
            '../../src/ppc/frames-ppc.cc',
            '../../src/ppc/frames-ppc.h',
            '../../src/ppc/interface-descriptors-ppc.cc',
            '../../src/ppc/interface-descriptors-ppc.h',
            '../../src/ppc/macro-assembler-ppc.cc',
            '../../src/ppc/macro-assembler-ppc.h',
            '../../src/ppc/simulator-ppc.cc',
            '../../src/ppc/simulator-ppc.h',
            '../../src/regexp/ppc/regexp-macro-assembler-ppc.cc',
            '../../src/regexp/ppc/regexp-macro-assembler-ppc.h',
          ],
        }],
        ['OS=="win"', {
          'variables': {
            'gyp_generators': '<!(echo $GYP_GENERATORS)',
          },
          'msvs_disabled_warnings': [4351, 4355, 4800],
          # When building Official, the .lib is too large and exceeds the 2G
          # limit. This breaks it into multiple pieces to avoid the limit.
          # See http://crbug.com/485155.
          'msvs_shard': 4,
          'cflags': [
            '/Zc:sizedDealloc-'
          ]
        }],
        ['component=="shared_library"', {
          'defines': [
            'BUILDING_V8_SHARED',
            'V8_SHARED',
          ],
        }],
        ['v8_postmortem_support=="true"', {
          'sources': [
            '<(SHARED_INTERMEDIATE_DIR)/debug-support.cc',
          ]
        }],
        ['v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icui18n',
            '<(icu_gyp_path):icuuc',
          ]
        }, {  # v8_enable_i18n_support==0
          'sources!': [
            '../../src/i18n.cc',
            '../../src/i18n.h',
          ],
        }],
        ['OS=="win" and v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icudata',
          ],
        }],
        ['icu_use_data_file_flag==1', {
          'defines': ['ICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE'],
        }, { # else icu_use_data_file_flag !=1
          'conditions': [
            ['OS=="win"', {
              'defines': ['ICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_SHARED'],
            }, {
              'defines': ['ICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_STATIC'],
            }],
          ],
        }],
      ],
    },
    {
      'target_name': 'v8_libbase',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'static_library',
      'variables': {
        'optimize': 'max',
      },
      'include_dirs+': [
        '../..',
      ],
      'sources': [
        '../../src/base/adapters.h',
        '../../src/base/atomicops.h',
        '../../src/base/atomicops_internals_arm64_gcc.h',
        '../../src/base/atomicops_internals_arm_gcc.h',
        '../../src/base/atomicops_internals_atomicword_compat.h',
        '../../src/base/atomicops_internals_mac.h',
        '../../src/base/atomicops_internals_mips_gcc.h',
        '../../src/base/atomicops_internals_mips64_gcc.h',
        '../../src/base/atomicops_internals_portable.h',
        '../../src/base/atomicops_internals_ppc_gcc.h',
        '../../src/base/atomicops_internals_s390_gcc.h',
        '../../src/base/atomicops_internals_tsan.h',
        '../../src/base/atomicops_internals_x86_gcc.cc',
        '../../src/base/atomicops_internals_x86_gcc.h',
        '../../src/base/atomicops_internals_x86_msvc.h',
        '../../src/base/bits.cc',
        '../../src/base/bits.h',
        '../../src/base/build_config.h',
        '../../src/base/compiler-specific.h',
        '../../src/base/cpu.cc',
        '../../src/base/cpu.h',
        '../../src/base/division-by-constant.cc',
        '../../src/base/division-by-constant.h',
        '../../src/base/flags.h',
        '../../src/base/functional.cc',
        '../../src/base/functional.h',
        '../../src/base/iterator.h',
        '../../src/base/lazy-instance.h',
        '../../src/base/logging.cc',
        '../../src/base/logging.h',
        '../../src/base/macros.h',
        '../../src/base/once.cc',
        '../../src/base/once.h',
        '../../src/base/platform/elapsed-timer.h',
        '../../src/base/platform/time.cc',
        '../../src/base/platform/time.h',
        '../../src/base/platform/condition-variable.cc',
        '../../src/base/platform/condition-variable.h',
        '../../src/base/platform/mutex.cc',
        '../../src/base/platform/mutex.h',
        '../../src/base/platform/platform.h',
        '../../src/base/platform/semaphore.cc',
        '../../src/base/platform/semaphore.h',
        '../../src/base/safe_conversions.h',
        '../../src/base/safe_conversions_impl.h',
        '../../src/base/safe_math.h',
        '../../src/base/safe_math_impl.h',
        '../../src/base/smart-pointers.h',
        '../../src/base/sys-info.cc',
        '../../src/base/sys-info.h',
        '../../src/base/utils/random-number-generator.cc',
        '../../src/base/utils/random-number-generator.h',
      ],
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
        }, {
          'toolsets': ['target'],
        }],
        ['OS=="linux"', {
            'conditions': [
              ['nacl_target_arch=="none"', {
                'link_settings': {
                  'libraries': [
                    '-ldl',
                    '-lrt'
                  ],
                },
              }, {
                'defines': [
                  'V8_LIBRT_NOT_AVAILABLE=1',
                ],
              }],
            ],
            'sources': [
              '../../src/base/platform/platform-linux.cc',
              '../../src/base/platform/platform-posix.cc'
            ],
          }
        ],
        ['OS=="android"', {
            'sources': [
              '../../src/base/platform/platform-posix.cc'
            ],
            'link_settings': {
              'target_conditions': [
                ['_toolset=="host"', {
                  # Only include libdl and librt on host builds because they
                  # are included by default on Android target builds, and we
                  # don't want to re-include them here since this will change
                  # library order and break (see crbug.com/469973).
                  'libraries': [
                    '-ldl',
                    '-lrt'
                  ]
                }]
              ]
            },
            'conditions': [
              ['host_os=="mac"', {
                'target_conditions': [
                  ['_toolset=="host"', {
                    'sources': [
                      '../../src/base/platform/platform-macos.cc'
                    ]
                  }, {
                    'sources': [
                      '../../src/base/platform/platform-linux.cc'
                    ]
                  }],
                ],
              }, {
                'sources': [
                  '../../src/base/platform/platform-linux.cc'
                ]
              }],
            ],
          },
        ],
        ['OS=="qnx"', {
            'link_settings': {
              'target_conditions': [
                ['_toolset=="host" and host_os=="linux"', {
                  'libraries': [
                    '-lrt'
                  ],
                }],
                ['_toolset=="target"', {
                  'libraries': [
                    '-lbacktrace'
                  ],
                }],
              ],
            },
            'sources': [
              '../../src/base/platform/platform-posix.cc',
              '../../src/base/qnx-math.h',
            ],
            'target_conditions': [
              ['_toolset=="host" and host_os=="linux"', {
                'sources': [
                  '../../src/base/platform/platform-linux.cc'
                ],
              }],
              ['_toolset=="host" and host_os=="mac"', {
                'sources': [
                  '../../src/base/platform/platform-macos.cc'
                ],
              }],
              ['_toolset=="target"', {
                'sources': [
                  '../../src/base/platform/platform-qnx.cc'
                ],
              }],
            ],
          },
        ],
        ['OS=="freebsd"', {
            'link_settings': {
              'libraries': [
                '-L/usr/local/lib -lexecinfo',
            ]},
            'sources': [
              '../../src/base/platform/platform-freebsd.cc',
              '../../src/base/platform/platform-posix.cc'
            ],
          }
        ],
        ['OS=="openbsd"', {
            'link_settings': {
              'libraries': [
                '-L/usr/local/lib -lexecinfo',
            ]},
            'sources': [
              '../../src/base/platform/platform-openbsd.cc',
              '../../src/base/platform/platform-posix.cc'
            ],
          }
        ],
        ['OS=="netbsd"', {
            'link_settings': {
              'libraries': [
                '-L/usr/pkg/lib -Wl,-R/usr/pkg/lib -lexecinfo',
            ]},
            'sources': [
              '../../src/base/platform/platform-openbsd.cc',
              '../../src/base/platform/platform-posix.cc'
            ],
          }
        ],
        ['OS=="aix"', {
          'sources': [
            '../../src/base/platform/platform-aix.cc',
            '../../src/base/platform/platform-posix.cc'
          ]},
        ],
        ['OS=="solaris"', {
            'link_settings': {
              'libraries': [
                '-lnsl -lrt',
            ]},
            'sources': [
              '../../src/base/platform/platform-solaris.cc',
              '../../src/base/platform/platform-posix.cc'
            ],
          }
        ],
        ['OS=="mac"', {
          'sources': [
            '../../src/base/platform/platform-macos.cc',
            '../../src/base/platform/platform-posix.cc'
          ]},
        ],
        ['OS=="win"', {
          'defines': [
            '_CRT_RAND_S'  # for rand_s()
          ],
          'variables': {
            'gyp_generators': '<!(echo $GYP_GENERATORS)',
          },
          'conditions': [
            ['gyp_generators=="make"', {
              'variables': {
                'build_env': '<!(uname -o)',
              },
              'conditions': [
                ['build_env=="Cygwin"', {
                  'sources': [
                    '../../src/base/platform/platform-cygwin.cc',
                    '../../src/base/platform/platform-posix.cc'
                  ],
                }, {
                  'sources': [
                    '../../src/base/platform/platform-win32.cc',
                    '../../src/base/win32-headers.h',
                  ],
                }],
              ],
              'link_settings':  {
                'libraries': [ '-lwinmm', '-lws2_32' ],
              },
            }, {
              'sources': [
                '../../src/base/platform/platform-win32.cc',
                '../../src/base/win32-headers.h',
              ],
              'msvs_disabled_warnings': [4351, 4355, 4800],
              'link_settings':  {
                'libraries': [ '-lwinmm.lib', '-lws2_32.lib' ],
              },
            }],
          ],
        }],
      ],
    },
    {
      'target_name': 'v8_libplatform',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'static_library',
      'variables': {
        'optimize': 'max',
      },
      'dependencies': [
        'v8_libbase',
      ],
      'include_dirs+': [
        '../..',
      ],
      'sources': [
        '../../include/libplatform/libplatform.h',
        '../../src/libplatform/default-platform.cc',
        '../../src/libplatform/default-platform.h',
        '../../src/libplatform/task-queue.cc',
        '../../src/libplatform/task-queue.h',
        '../../src/libplatform/worker-thread.cc',
        '../../src/libplatform/worker-thread.h',
      ],
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
        }, {
          'toolsets': ['target'],
        }],
      ],
    },
    {
      'target_name': 'natives_blob',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'none',
      'conditions': [
        [ 'v8_use_external_startup_data==1', {
          'conditions': [
            ['want_separate_host_toolset==1', {
              'dependencies': ['js2c#host'],
            }, {
              'dependencies': ['js2c'],
            }],
          ],
          'actions': [{
            'action_name': 'concatenate_natives_blob',
            'inputs': [
              '../../tools/concatenate-files.py',
              '<(SHARED_INTERMEDIATE_DIR)/libraries.bin',
              '<(SHARED_INTERMEDIATE_DIR)/libraries-experimental.bin',
              '<(SHARED_INTERMEDIATE_DIR)/libraries-extras.bin',
              '<(SHARED_INTERMEDIATE_DIR)/libraries-experimental-extras.bin',
            ],
            'conditions': [
              ['want_separate_host_toolset==1', {
                'target_conditions': [
                  ['_toolset=="host"', {
                    'outputs': [
                      '<(PRODUCT_DIR)/natives_blob_host.bin',
                    ],
                    'action': [
                      'python', '<@(_inputs)', '<(PRODUCT_DIR)/natives_blob_host.bin'
                    ],
                  }, {
                    'outputs': [
                      '<(PRODUCT_DIR)/natives_blob.bin',
                    ],
                    'action': [
                      'python', '<@(_inputs)', '<(PRODUCT_DIR)/natives_blob.bin'
                    ],
                  }],
                ],
              }, {
                'outputs': [
                  '<(PRODUCT_DIR)/natives_blob.bin',
                ],
                'action': [
                  'python', '<@(_inputs)', '<(PRODUCT_DIR)/natives_blob.bin'
                ],
              }],
            ],
          }],
        }],
        ['want_separate_host_toolset==1', {
          'toolsets': ['host', 'target'],
        }, {
          'toolsets': ['target'],
        }],
      ]
    },
    {
      'target_name': 'js2c',

      # Artillery fix -- There seems to be no way to specify this other than here.
      'default_configuration': 'Release',
      'configurations': {
        'Debug': {
          'defines': [ 'DEBUG', '_DEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 3, # multi-threaded DLL debug
            },
          },
        },
        'Release': {
          'defines': [ 'NDEBUG' ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeLibrary': 2, # multi-threaded DLL release
            },
          },
        },
      },

      'type': 'none',
      'conditions': [
        ['want_separate_host_toolset==1', {
          'toolsets': ['host'],
        }, {
          'toolsets': ['target'],
        }],
        ['v8_enable_i18n_support==1', {
          'variables': {
            'i18n_library_files': [
              '../../src/js/i18n.js',
            ],
          },
        }, {
          'variables': {
            'i18n_library_files': [],
          },
        }],
      ],
      'variables': {
        'library_files': [
          '../../src/js/macros.py',
          '../../src/messages.h',
          '../../src/js/prologue.js',
          '../../src/js/runtime.js',
          '../../src/js/v8natives.js',
          '../../src/js/symbol.js',
          '../../src/js/array.js',
          '../../src/js/string.js',
          '../../src/js/uri.js',
          '../../src/js/math.js',
          '../../src/third_party/fdlibm/fdlibm.js',
          '../../src/js/regexp.js',
          '../../src/js/arraybuffer.js',
          '../../src/js/typedarray.js',
          '../../src/js/iterator-prototype.js',
          '../../src/js/generator.js',
          '../../src/js/object-observe.js',
          '../../src/js/collection.js',
          '../../src/js/weak-collection.js',
          '../../src/js/collection-iterator.js',
          '../../src/js/promise.js',
          '../../src/js/messages.js',
          '../../src/js/json.js',
          '../../src/js/array-iterator.js',
          '../../src/js/string-iterator.js',
          '../../src/js/templates.js',
          '../../src/js/spread.js',
          '../../src/debug/mirrors.js',
          '../../src/debug/debug.js',
          '../../src/debug/liveedit.js',
        ],
        'experimental_library_files': [
          '../../src/js/macros.py',
          '../../src/messages.h',
          '../../src/js/proxy.js',
          '../../src/js/generator.js',
          '../../src/js/harmony-atomics.js',
          '../../src/js/harmony-regexp.js',
          '../../src/js/harmony-object-observe.js',
          '../../src/js/harmony-sharedarraybuffer.js',
          '../../src/js/harmony-simd.js',
          '../../src/js/harmony-species.js',
          '../../src/js/harmony-unicode-regexps.js',
          '../../src/js/promise-extra.js',
        ],
        'libraries_bin_file': '<(SHARED_INTERMEDIATE_DIR)/libraries.bin',
        'libraries_experimental_bin_file': '<(SHARED_INTERMEDIATE_DIR)/libraries-experimental.bin',
        'libraries_extras_bin_file': '<(SHARED_INTERMEDIATE_DIR)/libraries-extras.bin',
        'libraries_experimental_extras_bin_file': '<(SHARED_INTERMEDIATE_DIR)/libraries-experimental-extras.bin',
      },
      'actions': [
        {
          'action_name': 'js2c',
          'inputs': [
            '../../tools/js2c.py',
            '<@(library_files)',
            '<@(i18n_library_files)'
          ],
          'outputs': ['<(SHARED_INTERMEDIATE_DIR)/libraries.cc'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/libraries.cc',
            'CORE',
            '<@(library_files)',
            '<@(i18n_library_files)'
          ],
        },
        {
          'action_name': 'js2c_bin',
          'inputs': [
            '../../tools/js2c.py',
            '<@(library_files)',
            '<@(i18n_library_files)'
          ],
          'outputs': ['<@(libraries_bin_file)'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/libraries.cc',
            'CORE',
            '<@(library_files)',
            '<@(i18n_library_files)',
            '--startup_blob', '<@(libraries_bin_file)',
            '--nojs',
          ],
        },
        {
          'action_name': 'js2c_experimental',
          'inputs': [
            '../../tools/js2c.py',
            '<@(experimental_library_files)',
          ],
          'outputs': ['<(SHARED_INTERMEDIATE_DIR)/experimental-libraries.cc'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/experimental-libraries.cc',
            'EXPERIMENTAL',
            '<@(experimental_library_files)'
          ],
        },
        {
          'action_name': 'js2c_experimental_bin',
          'inputs': [
            '../../tools/js2c.py',
            '<@(experimental_library_files)',
          ],
          'outputs': ['<@(libraries_experimental_bin_file)'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/experimental-libraries.cc',
            'EXPERIMENTAL',
            '<@(experimental_library_files)',
            '--startup_blob', '<@(libraries_experimental_bin_file)',
            '--nojs',
          ],
        },
        {
          'action_name': 'js2c_extras',
          'inputs': [
            '../../tools/js2c.py',
            '<@(v8_extra_library_files)',
          ],
          'outputs': ['<(SHARED_INTERMEDIATE_DIR)/extras-libraries.cc'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/extras-libraries.cc',
            'EXTRAS',
            '<@(v8_extra_library_files)',
          ],
        },
        {
          'action_name': 'js2c_extras_bin',
          'inputs': [
            '../../tools/js2c.py',
            '<@(v8_extra_library_files)',
          ],
          'outputs': ['<@(libraries_extras_bin_file)'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/extras-libraries.cc',
            'EXTRAS',
            '<@(v8_extra_library_files)',
            '--startup_blob', '<@(libraries_extras_bin_file)',
            '--nojs',
          ],
        },
        {
          'action_name': 'js2c_experimental_extras',
          'inputs': [
            '../../tools/js2c.py',
            '<@(v8_experimental_extra_library_files)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/experimental-extras-libraries.cc',
          ],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/experimental-extras-libraries.cc',
            'EXPERIMENTAL_EXTRAS',
            '<@(v8_experimental_extra_library_files)',
          ],
        },
        {
          'action_name': 'js2c_experimental_extras_bin',
          'inputs': [
            '../../tools/js2c.py',
            '<@(v8_experimental_extra_library_files)',
          ],
          'outputs': ['<@(libraries_experimental_extras_bin_file)'],
          'action': [
            'python',
            '../../tools/js2c.py',
            '<(SHARED_INTERMEDIATE_DIR)/experimental-extras-libraries.cc',
            'EXPERIMENTAL_EXTRAS',
            '<@(v8_experimental_extra_library_files)',
            '--startup_blob', '<@(libraries_experimental_extras_bin_file)',
            '--nojs',
          ],
        },
      ],
    },
    {
      'target_name': 'postmortem-metadata',
      'type': 'none',
      'variables': {
        'heapobject_files': [
            '../../src/objects.h',
            '../../src/objects-inl.h',
        ],
      },
      'actions': [
          {
            'action_name': 'gen-postmortem-metadata',
            'inputs': [
              '../../tools/gen-postmortem-metadata.py',
              '<@(heapobject_files)',
            ],
            'outputs': [
              '<(SHARED_INTERMEDIATE_DIR)/debug-support.cc',
            ],
            'action': [
              'python',
              '../../tools/gen-postmortem-metadata.py',
              '<@(_outputs)',
              '<@(heapobject_files)'
            ]
          }
        ]
    },
    {
      'target_name': 'mksnapshot',
      'type': 'executable',
      'dependencies': ['v8_base', 'v8_nosnapshot', 'v8_libplatform'],
      'include_dirs+': [
        '../..',
      ],
      'sources': [
        '../../src/snapshot/mksnapshot.cc',
      ],
      'conditions': [
        ['v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icui18n',
            '<(icu_gyp_path):icuuc',
          ]
        }],
        ['want_separate_host_toolset==1', {
          'toolsets': ['host'],
        }, {
          'toolsets': ['target'],
        }],
      ],
    },
  ],
}
