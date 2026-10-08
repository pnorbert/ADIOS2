# SPDX-License-Identifier: Apache-2.0

gtest_add_tests_helper(Basic MPI_NONE WANStreamer Engine.WANStreamer. "")
set_tests_properties(${Test.Engine.WANStreamer.Basic-TESTS}
  PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
