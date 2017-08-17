# convenience macro to add qtest unit tests
macro(add_unittest _source _additionalsource)
    set(_test ${_source} ${_additionalsource})
    get_filename_component(_name ${_source} NAME_WE)
    add_executable( ${_name} ${_test} )
    add_test(NAME ${_name} COMMAND ${_name} )
    ecm_mark_as_test(akonadiconsole-${_name})
    set_tests_properties(${_name} PROPERTIES ENVIRONMENT "QT_HASH_SEED=1;QT_NO_CPU_FEATURE=sse4.2")
    target_link_libraries( ${_name}
        Qt5::Test
        KF5::I18n
        Qt5::Widgets
        KF5::ConfigCore
        KF5::WidgetsAddons
        KF5::ItemViews
        KF5::Completion
        libakonadiconsole
        )
endmacro ()

add_unittest(jobtrackertest.cpp "")
add_unittest(jobtrackermodeltest.cpp "")
add_unittest(jobtrackersearchwidgettest.cpp "")
