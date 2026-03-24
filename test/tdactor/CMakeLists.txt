add_executable(test-scheduler-stop test-scheduler-stop.cpp)

target_link_libraries(test-scheduler-stop PRIVATE
    tdutils
    tdactor
    td-test-main
)

ion_test(test-scheduler-stop)
