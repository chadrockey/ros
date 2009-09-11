rosbuild_find_ros_package(genmsg_cpp)

# Message-generation support.
macro(genmsg_java)
  get_msgs(_msglist)
  set(_autogen "")
  foreach(_msg ${_msglist})
    # Construct the path to the .msg file
    set(_input ${PROJECT_SOURCE_DIR}/msg/${_msg})
  
    gendeps(${PROJECT_NAME} ${_msg})
  
    set(genmsg_java_exe ${genmsg_cpp_PACKAGE_PATH}/genmsg_java)
  
    # TODO: Figure a better way to lay out the .java files
    set(_output_java ${PROJECT_SOURCE_DIR}/msg/java/ros/pkg/${PROJECT_NAME}/msg/${_msg})
    string(REPLACE ".msg" ".java" _output_java ${_output_java})
  
    # Add the rule to build the .java from the .msg
    add_custom_command(OUTPUT ${_output_java} 
                       COMMAND ${genmsg_java_exe} ${_input}
                       DEPENDS ${_input} ${genmsg_java_exe} ${gendeps_exe} ${${PROJECT_NAME}_${_msg}_GENDEPS} ${ROS_MANIFEST_LIST})
    list(APPEND _autogen ${_output_java})
  endforeach(_msg)
  # Create a target that depends on the union of all the autogenerated
  # files
  add_custom_target(ROSBUILD_genmsg_java DEPENDS ${_autogen})
  # Add our target to the top-level genmsg target, which will be fired if
  # the user calls genmsg()
  add_dependencies(rospack_genmsg ROSBUILD_genmsg_java)
endmacro(genmsg_java)

# Call the macro we just defined.
genmsg_java()

# Service-generation support.
macro(gensrv_java)
  get_srvs(_srvlist)
  set(_autogen "")
  foreach(_srv ${_srvlist})
    # Construct the path to the .srv file
    set(_input ${PROJECT_SOURCE_DIR}/srv/${_srv})
  
    gendeps(${PROJECT_NAME} ${_srv})
  
    set(gensrv_java_exe ${genmsg_cpp_PACKAGE_PATH}/gensrv_java)

    # TODO: Figure a better way to lay out the .java files
    set(_output_java ${PROJECT_SOURCE_DIR}/srv/java/ros/pkg/${PROJECT_NAME}/srv/${_srv})
  
    string(REPLACE ".srv" ".java" _output_java ${_output_java})
  
    # Add the rule to build the .java from the .srv
    add_custom_command(OUTPUT ${_output_java} 
                       COMMAND ${gensrv_java_exe} ${_input}
                       DEPENDS ${_input} ${gensrv_java_exe} ${gendeps_exe} ${${PROJECT_NAME}_${_srv}_GENDEPS} ${ROS_MANIFEST_LIST})
    list(APPEND _autogen ${_output_java})
  endforeach(_srv)
  # Create a target that depends on the union of all the autogenerated
  # files
  add_custom_target(ROSBUILD_gensrv_java DEPENDS ${_autogen})
  # Add our target to the top-level gensrv target, which will be fired if
  # the user calls gensrv()
  add_dependencies(rospack_gensrv ROSBUILD_gensrv_java)
endmacro(gensrv_java)


# Call the macro we just defined.
gensrv_java()

