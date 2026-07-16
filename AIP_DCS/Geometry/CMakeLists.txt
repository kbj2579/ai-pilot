set(SOURCES Vector4.cpp
            Vector3.cpp
            Quaternion.cpp
            Matrix4.cpp
            Matrix3.cpp
            Math.cpp
            EulerAngle.cpp
            CoordinateConverter.cpp
            AxisAngle.cpp
            Angle.cpp
            Controller_CY.cpp
            )

set(HEADERS Vector4.h
            Vector3.h
            Quaternion.h
            Matrix4.h
            Matrix3.h
            Math.h
            EulerAngle.h
            CoordinateConverter.h
            AxisAngle.h
            Angle.h         
            Controller_CY.h
           )

add_library(Geometry OBJECT ${HEADERS} ${SOURCES})
set_target_properties(Geometry PROPERTIES TARGET_DIRECTORY
                           ${CMAKE_CURRENT_SOURCE_DIR})

