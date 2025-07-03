/* ===========================================================================
** Copyright (C) 2024 Ezlo Innovation Inc
**
** Under EZLO AVAILABLE SOURCE LICENSE (EASL) AGREEMENT
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/
/**
 * @file    ezlopi_core_devices_list_modules.h
 * @brief   Macros to list device modeles
 * @author  xx
 * @version 0.1
 * @date    12th DEC 2024
*/

/*******************************************************************************
*                          Include Files
*******************************************************************************/


/*******************************************************************************
*                          C++ Declaration Wrapper
*******************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

        /*******************************************************************************
        *                          Type & Macro Declarations
        *******************************************************************************/

#if (1 == DEVICE_00102_BECA_BHT003_GCLW)
EZLOPI_DEVICE_LIST("device-beca-bht003                 ", 102, device_00102_BECA_BHT003_GCLW)
#endif // DEVICE_00102_BECA_BHT003_GCLW

#if (1 == DEVICE_00116_BECA_1000_3A)
EZLOPI_DEVICE_LIST("device-beca-10003A                 ", 116, device_00116_BECA_1000_3A)
#endif // DEVICE_00116_BECA_1000_3A


                //////// End of sensor/device list
                EZLOPI_DEVICE_LIST(NULL, 0, NULL)
                /*******************************************************************************
                *                          Extern Data Declarations
                *******************************************************************************/

                /*******************************************************************************
                *                          Extern Function Prototypes
                *******************************************************************************/


#ifdef __cplusplus
}
#endif


/*******************************************************************************
*                          End of File
*******************************************************************************/
