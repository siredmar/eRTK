
#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <Platform_Types.h>

typedef uint8 Std_ReturnType;

#define E_OK		 0x00
#define E_NOT_OK	 0x01

#define STD_HIGH     0x01     /**<\brief Physical state 5V or 3.3V */
#define STD_LOW      0x00     /**<\brief Physical state 0V */

#define STD_ACTIVE   0x01     /**<\brief Logical state active */
#define STD_IDLE     0x00     /**<\brief Logical state idle */

#define STD_ON       0x01     /**<\brief Standard ON type */
#define STD_OFF      0x00     /**<\brief Standard OFF type */

#define STD_ENABLE   0x01     /**<\brief Standard ENABLE type */
#define STD_DISABLE  0x00     /**<\brief Standard DISABLE type */

#endif /* STD_TYPES_H */


