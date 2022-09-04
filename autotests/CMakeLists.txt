# convenience macro to add qtest unit tests
macro(add_unittest _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE)
    add_executable( ${_name} ${_test}  ${_name}.h)
    add_test(NAME ${_name} COMMAND ${_name})
    ecm_mark_as_test(akonadiconsole-${_name})
    set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=1")
    target_link_libraries( ${_name}
        Qt::Test
        KF5::I18n
        Qt::Widgets
        KF5::ConfigCore
        KF5::WidgetsAddons
        KF5::ItemViews
        KF5::Completion
        libakonadiconsole
        )
endmacro ()

add_unittest(jobtrackertest.cpp)
add_unittest(jobtrackermodeltest.cpp )
add_unittest(jobtrackersearchwidgettest.cpp )
