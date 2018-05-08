/**
  ******************************************************************************
  * @file    Touch_Panel/main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    20-September-2013
  * @brief   This example describes how to configure and use the touch panel 
  *          mounted on STM32F429I-DISCO boards.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include <string.h>

/* Include core modules */
#include "stm32f4xx.h"
/* Include my libraries here */
#include "defines.h"
#include "usb_hid_device/tm_stm32f4_usb_hid_device.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_disco.h"

/** @addtogroup STM32F429I_DISCOVERY_Examples
  * @{
  */

/** @addtogroup Touch_Panel
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define abs(x) ((x)<0 ? -(x) : (x))

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void TP_Config(void);

/* Private functions ---------------------------------------------------------*/
void intToHexString(int intToConvert, char *hexString);

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  uint8_t already = 0;              //!< Indicates whether or not the board's USER button is already pressed
  uint8_t touching = 0;             //!< Indicates whether or not the TouchScreen is being touched
  uint8_t buttonsEnabled = 0;       //!< Indicates whether or not the graphical mouse button controls are enabled
  uint32_t counter = 0;             //!< Counts how many times the disabled graphical mouse button has been "pressed"/touched
  static TP_STATE* TP_State;        //!< Holds the TouchScreen state data
  TM_USB_HIDDEVICE_Mouse_t Mouse;   //!< Holds the HID device report data
  char hexString[7] = {0};          // 6 chars for 0x0000 and the last 0 to make it a NULL '\0' terminated string
  char tmpStr[25] = "Temp Str";     // Used for temporary string operations

  // Set up variables to keep track of the current and previous/last touch positions
  uint16_t currentX = 0, previousX = 0, currentY = 0, previousY = 0;
  uint16_t currentTouchDetected = 0, previousTouchDetected = 0;

  /* Initialize system */
  SystemInit();

  /* Initialize leds */
  TM_DISCO_LedInit();

  /* Initialize button */
  TM_DISCO_ButtonInit();

  /* Initialize delay */
  TM_DELAY_Init();

  /* Initialize USB HID Device */
  TM_USB_HIDDEVICE_Init();

  /* Set default values for mouse struct */
  TM_USB_HIDDEVICE_MouseStructInit(&Mouse);

  /* LCD initiatization */
  LCD_Init();
  
  /* LCD Layer initiatization */
  LCD_LayerInit();
    
  /* Enable the LTDC */
  LTDC_Cmd(ENABLE);
  
  /* Set LCD foreground layer */
  LCD_SetLayer(LCD_FOREGROUND_LAYER);
  
  /* Touch Panel configuration */
  TP_Config();

  LCD_SetFont(&Font8x8);
  LCD_SetBackColor(LCD_COLOR_BLACK);
  LCD_SetTextColor(LCD_COLOR_WHITE);
  LCD_DisplayStringLine(LINE(1), (uint8_t*)"Touch Coordinates:");

  while (1)
  {

    // Get the TouchScreen state
    TP_State = IOE_TP_GetState();

    // Check if the TouchScreen was touched
    if ( TP_State->TouchDetected ) {
      // Save the touch coordinates of this first touch
      previousX = TP_State->X;
      previousY = TP_State->Y;
      previousTouchDetected = TP_State->TouchDetected;

      // Wait some time to verify that the touchscreen is still being touched (touch duration). It seems that two consecutive calls to IOE_TP_GetState() cannot be made
      Delayms(8);
      TP_State = IOE_TP_GetState();

      // Check if TouchScreen is still touched
      if ( (TP_State->TouchDetected == 0) ) {
        // The touchscreen is no longer being touched after detecting the 1st touch, waiting and checking again. This could be a "tap" / mouse click (depends on the touch duration)
        touching = 0;
        TM_DISCO_LedToggle(LED_RED);
      }
      else {
        // TouchScreen is still being touched. Proceed to move mouse pointer
        touching = 1;

        if ( TP_State->X == 0xEF ) {
          // 0xEF is the max display x resolution x = 240 = 0xF0,
        }
        else {
          // Save the latest touchscreen data as the current state
          currentX = TP_State->X;
          currentY = TP_State->Y;
          currentTouchDetected = TP_State->TouchDetected;
        }
      }
    }
    else {
      touching = 0;
    }

    // Check if the USB device is connected
    if ( TM_USB_HIDDEVICE_GetStatus() == TM_USB_HIDDEVICE_Status_Connected ) {
      // Turn ON the STM32F429I-DISC1 discovery board's GREEN LED
      TM_DISCO_LedOn(LED_GREEN);

      if ( touching == 0 ) {
        // TouchScreen is NOT being touched. Do nothing?
      }
      else {
        // TouchScreen is being touched. Move mouse pointer
        // Only move the mouse pointer IF the TouchScreen graphical mouse button controls are NOT enabled
        if ( buttonsEnabled == 0 ) {
          // Move the mouse pointer if there is a significant difference between the last two consecutive TouchScreen states
          if ( abs(currentX - previousX) > 2 ) {
            Mouse.XAxis = (currentX - previousX)%0xE0; // X screen resolution is 240 (from 0 to 239); in hex 0xF0 (from 0 to 0xEF)
          }
          else {
            Mouse.XAxis = 0;
          }
          if ( abs(currentY - previousY) > 2 ) {
            Mouse.YAxis = (currentY - previousY)%0x130; // Y screen resolution is 320 (from 0 to 319); in hex 0x140 (from 0 to 0x13F)
          }
          else {
            Mouse.YAxis = 0;
          }
          Mouse.Wheel = 0;
        }
      }
      // END of if...else TouchScreen beign touched

      // Check if the board's USER button is pressed
      if ( TM_DISCO_ButtonPressed() && already == 0 ) {
        // The button is being pressed & was NOT already/previously pressed
        already = 1;

        // Turn ON the RED LED
        TM_DISCO_LedOn(LED_RED);

        // Send a mouse left button click/press. For some reason, 0 or TM_USB_HIDDEVICE_Button_Pressed DOES NOT WORK!!!
        Mouse.LeftButton = 0x9;

        // Do not send any mouse X or Y relative movement.
        Mouse.XAxis = 0;
        Mouse.YAxis = 0;

        // Send the USB HID device report
        TM_USB_HIDDEVICE_MouseSend(&Mouse);

        // Wait to avoid sending HID reports too fast which causes erratic behaviour on the mouse (erratic movement and random clicks)
        Delayms(10);

      }
      else if ( !TM_DISCO_ButtonPressed() && already == 1 ) {
        // The button is NOT being pressed & WAS already/previously pressed
        already = 0;

        // Turn ON the RED LED
        TM_DISCO_LedOff(LED_RED);

        // Send a mouse left, right & middle buttons release
        Mouse.LeftButton = TM_USB_HIDDEVICE_Button_Released;
        Mouse.RightButton = TM_USB_HIDDEVICE_Button_Released;
        Mouse.MiddleButton = TM_USB_HIDDEVICE_Button_Released;

        // Do not send any mouse X or Y relative movement.
        Mouse.XAxis = 0;
        Mouse.YAxis = 0;

        // Send the USB HID device report
        TM_USB_HIDDEVICE_MouseSend(&Mouse);

        // Wait to avoid sending HID reports too fast which causes erratic behaviour on the mouse (erratic movement and random clicks)
        Delayms(10);
      }

      // Display data in the LCD
      strcpy( tmpStr, "X: " );
      intToHexString( TP_State->X, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " Y: " );
      intToHexString( TP_State->Y, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " Z: " );
      intToHexString( TP_State->Z, hexString );
      strcat( tmpStr, hexString );
      LCD_SetTextColor( LCD_COLOR_CYAN );
      LCD_DisplayStringLine( LINE(2), (uint8_t*)tmpStr );

      strcpy( tmpStr, "Mouse coordinates: " );
      LCD_SetTextColor( LCD_COLOR_WHITE );
      LCD_DisplayStringLine( LINE(4), (uint8_t*)tmpStr );

      strcpy( tmpStr, "X: " );
      intToHexString( Mouse.XAxis, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " Y: " );
      intToHexString( Mouse.YAxis, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " W: " );
      intToHexString( Mouse.Wheel, hexString );
      strcat( tmpStr, hexString );
      LCD_SetTextColor( LCD_COLOR_CYAN );
      LCD_DisplayStringLine( LINE(5), (uint8_t*)tmpStr );

      strcpy( tmpStr, "L: " );
      intToHexString( Mouse.LeftButton, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " R: " );
      intToHexString( Mouse.RightButton, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " M: " );
      intToHexString( Mouse.MiddleButton, hexString );
      strcat( tmpStr, hexString );
      LCD_SetTextColor( LCD_COLOR_CYAN );
      LCD_DisplayStringLine( LINE(6), (uint8_t*)tmpStr );

      strcpy( tmpStr, "currX: " );
      intToHexString( currentX, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " prevX: " );
      intToHexString( previousX, hexString );
      strcat( tmpStr, hexString );
      LCD_SetTextColor( LCD_COLOR_GREEN );
      LCD_DisplayStringLine( LINE(8), (uint8_t*)tmpStr );

      strcpy( tmpStr, "currY: " );
      intToHexString( currentY, hexString );
      strcat( tmpStr, hexString );
      strcat( tmpStr, " prevY: " );
      intToHexString( previousY, hexString );
      strcat( tmpStr, hexString );
      LCD_SetTextColor( LCD_COLOR_GREEN );
      LCD_DisplayStringLine( LINE(9), (uint8_t*)tmpStr );

      // Check if the TouchScreen is being touched.
      if ( touching == 1 ) {

        // Check if the graphical buttons are enabled
        if (buttonsEnabled == 0) {
          // Send the USB HID device report
          TM_USB_HIDDEVICE_MouseSend(&Mouse);

          // Wait to avoid sending HID reports too fast which causes erratic behaviour on the mouse (erratic movement and random clicks)
          Delayms(10);
        }


        // Check if the graphical "Disable" button is being "pressed"
        if ( (TP_State->X >= 105) && (TP_State->X <= 135) && (TP_State->Y >= 215) && (TP_State->Y <= 245) ) {
          // Increase the number of times the "disable" graphical button has been "pressed"
          counter++;

          // Check if the
          if (counter >= 100) {

            // Check if the buttons are enabled
            if (buttonsEnabled == 1) {
              // Disable the mouse's graphical buttons
              buttonsEnabled = 0;

              // Redraw the buttons with DISABLED color: RED
              LCD_SetTextColor(LCD_COLOR_RED);
              LCD_DrawFullRect(0, 200, 60, 60);       // Left
              LCD_DrawFullRect(90, 140, 60, 60);      // Up
              LCD_DrawFullRect(180, 200, 60, 60); // Right
              LCD_DrawFullRect(90, 260, 60, 60); // Down
              LCD_DrawFullRect(105, 215, 30, 30); // Disable

              LCD_SetFont(&Font16x24);
              LCD_SetTextColor(LCD_COLOR_BLACK);
              LCD_SetBackColor(LCD_COLOR_RED);
              LCD_DisplayChar(LCD_LINE_9, 22, 0x3C);
              LCD_DisplayChar(LCD_LINE_7, 112, 0x5E);
              LCD_DisplayChar(LCD_LINE_9, 202, 0x3E);
              LCD_DisplayChar(LCD_LINE_12, 112, 0x3D);
              LCD_DisplayChar(LCD_LINE_9, 112, 0x58);
            }
            else {
              // Enable the mouse's graphical buttons
              buttonsEnabled = 1;

              // Redraw the buttons with ENABLED color: GREEN
              LCD_SetTextColor(LCD_COLOR_GREEN);
              LCD_DrawFullRect(0, 200, 60, 60);       // Left
              LCD_DrawFullRect(90, 140, 60, 60);      // Up
              LCD_DrawFullRect(180, 200, 60, 60); // Right
              LCD_DrawFullRect(90, 260, 60, 60); // Down
              LCD_DrawFullRect(105, 215, 30, 30); // Disable

              LCD_SetFont(&Font16x24);
              LCD_SetTextColor(LCD_COLOR_BLACK);
              LCD_SetBackColor(LCD_COLOR_GREEN);
              LCD_DisplayChar(LCD_LINE_9, 22, 0x3C);
              LCD_DisplayChar(LCD_LINE_7, 112, 0x5E);
              LCD_DisplayChar(LCD_LINE_9, 202, 0x3E);
              LCD_DisplayChar(LCD_LINE_12, 112, 0x3D);
              LCD_DisplayChar(LCD_LINE_9, 112, 0x58);
            }

            // Reset the "disabled" graphical button counter
            counter = 0;

            // Reset the LCD settings to display data
            LCD_SetFont(&Font8x8);
            LCD_SetTextColor(LCD_COLOR_WHITE);
            LCD_SetBackColor(LCD_COLOR_BLACK);
          }
        }

        // Check which mouse graphical button is being touched
        // Reset the mouse's X & Y relative movement
        Mouse.XAxis = 0;
        Mouse.YAxis = 0;

        // Touching LEFT button
        if ( (TP_State->X >= 0) && (TP_State->X <= 60) && (TP_State->Y >= 200) && (TP_State->Y <= 260) ) {
          Mouse.XAxis = -10;
          Mouse.YAxis = 0;
        }

        // Touching UP button
        if ( (TP_State->X >= 90) && (TP_State->X <= 150) && (TP_State->Y >= 140) && (TP_State->Y <= 200) ) {
          Mouse.XAxis = 0;
          Mouse.YAxis = -10;
        }

        // Touching RIGHT button
        if ( (TP_State->X >= 180) && (TP_State->X <= 240) && (TP_State->Y >= 200) && (TP_State->Y <= 260) ) {
          Mouse.XAxis = 10;
          Mouse.YAxis = 0;
        }

        // Touching DOWN button
        if ( (TP_State->X >= 90) && (TP_State->X <= 150) && (TP_State->Y >= 260) && (TP_State->Y <= 320) ) {
          Mouse.XAxis = 0;
          Mouse.YAxis = 10;
        }

        // Check if the mouse's graphical buttons are enabled
        if ( buttonsEnabled == 1 ) {
          TM_USB_HIDDEVICE_MouseSend(&Mouse);
          Delayms(10);
        }

      }
      // END if ( touching == 1 ) {

    }
    else {
      // USB HID device IS NOT connected
      // Turn OFF GREEN LED
      TM_DISCO_LedOff(LED_GREEN);
    }
  }
}



/**
 * @brief   Converts an integer into a 4 digit HEX string with "0x" as prefix / prepended
 * @param   intToConvert:   The integer to convert into a HEX string
 * @param   *hexString:     Pointer to the char array/string which will contain the
 * resulting HEX string. This array MUST have a size of at least 7 chars.
 * @retval  None
 */
void intToHexString(int intToConvert, char *hexString) {
  uint8_t intDigit = 0;
  char ascii = ' ';

  // Prepend "0x" to the 4 digit HEX string
  hexString[0] = '0';
  hexString[1] = 'x';

  for (int i = 0; i < 4; i++) {
    intDigit = (intToConvert >> (i * 4)) & 0xF;

    switch(intDigit) {
      case 0x0:
        ascii = '0';
        break;
      case 0x1:
        ascii = '1';
        break;
      case 0x2:
        ascii = '2';
        break;
      case 0x3:
        ascii = '3';
        break;
      case 0x4:
        ascii = '4';
        break;
      case 0x5:
        ascii = '5';
        break;
      case 0x6:
        ascii = '6';
        break;
      case 0x7:
        ascii = '7';
        break;
      case 0x8:
        ascii = '8';
        break;
      case 0x9:
        ascii = '9';
        break;
      case 0xA:
        ascii = 'A';
        break;
      case 0xB:
        ascii = 'B';
        break;
      case 0xC:
        ascii = 'C';
        break;
      case 0xD:
        ascii = 'D';
        break;
      case 0xE:
        ascii = 'E';
        break;
      case 0xF:
        ascii = 'F';
        break;
    }

    hexString[5 - i] = ascii;

    // Set the NULL '\0' character at the end of the string
    hexString[6] = 0;
  }

}



/**
* @brief  Configure the IO Expander and the Touch Panel.
* @param  None
* @retval None
*/
static void TP_Config(void)
{
  /* Clear the LCD */ 
//  LCD_Clear(LCD_COLOR_WHITE);
  LCD_Clear(LCD_COLOR_BLACK);
  LCD_SetBackColor(LCD_COLOR_BLACK);
  
  /* Configure the IO Expander */
  if (IOE_Config() == IOE_OK)
  {
    LCD_SetTextColor(LCD_COLOR_RED);
    LCD_DrawFullRect(0, 200, 60, 60);       // Left
    LCD_DrawFullRect(90, 140, 60, 60);      // Up
    LCD_DrawFullRect(180, 200, 60, 60); // Right
    LCD_DrawFullRect(90, 260, 60, 60); // Down
    LCD_DrawFullRect(105, 215, 30, 30); // Disable


    LCD_SetFont(&Font16x24);
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_SetBackColor(LCD_COLOR_RED);
    LCD_DisplayChar(LCD_LINE_9, 22, 0x3C);
    LCD_DisplayChar(LCD_LINE_7, 112, 0x5E);
    LCD_DisplayChar(LCD_LINE_9, 202, 0x3E);
    LCD_DisplayChar(LCD_LINE_12, 112, 0x3D);
    LCD_DisplayChar(LCD_LINE_9, 112, 0x58);
  }  
  else
  {
    LCD_Clear(LCD_COLOR_RED);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"   IOE NOT OK      ");
    LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"Reset the board   ");
    LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"and try again     ");
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
