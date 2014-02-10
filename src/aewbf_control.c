#include "alg_aewb_priv.h"
#include <drv_gpio.h>
#include <drv_ipipe.h>
#include <drv_isif.h>

#include "aewbf_sig.h"
#include "iaewbf_sig.h"

ALG_AewbObj gALG_aewbObj;
ALG_AewbfObj gSIG_Obj;

extern DRV_IpipeObj gDRV_ipipeObj;    //For boxcar
extern CSL_IpipeObj gCSL_ipipeHndl;   //For gamma and rgb2rgb

extern int gAePriorityMode, gBWMode, gDayNight, gIRCut, defaultFPS, gFlicker;
extern int IRcutClose, FPShigh;
extern Uint32 gamma42[], gamma00520[], gamma_hdr011[], gamma_hdr01[], gamma01[], gamma005[], gamma003[];

int Get_BoxCar(IALG_Handle handle)
{
    int status;
    int bufId=-1;
    IAEWBF_SIG_Obj *hn = (IAEWBF_SIG_Obj *)handle;
    OSA_BufInfo *pBufInfo;

    status = DRV_ipipeGetBoxcarBuf(&bufId, OSA_TIMEOUT_NONE);
    if(status!= OSA_SOK) {
        OSA_ERROR("ERROR: DRV_ipipeGetBoxcarBuf()\n");
        return status;
    }
    pBufInfo = DRV_ipipeGetBoxcarBufInfo(bufId);
    DRV_ipipePutBoxcarBuf(bufId);

    //Fill AEWBF filds
    hn->box = pBufInfo->virtAddr;
    hn->w = gDRV_ipipeObj.boxcarInfo.width;
    hn->h = gDRV_ipipeObj.boxcarInfo.height;
    hn->SatTh = hn->w*hn->h*3/300;

    return OSA_SOK;
}

void ALG_SIG_config(IALG_Handle handle)
{
    IAEWBF_SIG_Obj *hn = (IAEWBF_SIG_Obj *)handle;

    //Uint32 tables[512];
    CSL_IpipeGammaConfig dataG;
    CSL_IpipeRgb2RgbConfig rgb2rgb1, rgb2rgb2;
    DRV_IpipeWb ipipeWb;

    DRV_imgsSetEshutter(hn->Exp.Range.max, 0); //Max expouse
    DRV_imgsSetFramerate(defaultFPS); //Max FPS frame rate
    ALG_aewbSetSensorDcsub(176); //176 Offset for SONY IMX136
    //ALG_aewbSetSensorDcsub(170); //170 Offset for Aptina MT9P006
    //DRV_imgsNDShutterInit();
    //DRV_imgsNDShutter(1, gBWMode); //Close IR-cut

    //DRV_imgsSetCompress(9, 10, 1, 6);
    //DRV_imgsSetCompress(8, 10, 2, 5);
    //DRV_imgsSetCompress(6, 12, 3, 6);

    //Config contrast and Brightness
    //DRV_ipipeSetYoffet((pParm->yuv_adj_brt-128));
    //DRV_ipipeSetContrastBrightness(pParm->yuv_adj_ctr, 0x0);

    //Config gamma correction tables

    dataG.tableSize = CSL_IPIPE_GAMMA_CORRECTION_TABLE_SIZE_512;
    dataG.tableSrc  = CSL_IPIPE_GAMMA_CORRECTION_TABLE_SELECT_RAM;
    dataG.bypassR = 0;
    dataG.bypassG = dataG.bypassR;
    dataG.bypassB = dataG.bypassR;
    dataG.tableR = gamma005; //gamma005//gamma42; //gamma_hdr011; //gamma_hdr01; //gamma01; //gamma00520
    dataG.tableG = dataG.tableR;
    dataG.tableB = dataG.tableR;

    //Liner gamma tables
    /*
    vl0 = 0;
    for(i=0; i < 512; i++){
        vl1 = i<<1;
        tables[i] = (vl0<<10) | (vl1 - vl0);
        vl0 = vl1;
    }
    */

    if(CSL_ipipeSetGammaConfig(&gCSL_ipipeHndl, &dataG) != CSL_SOK)
        OSA_ERROR("Fail CSL_ipipeSetGammaConfig!!!\n");

    //Config RGB2RGB matrix

    rgb2rgb1.matrix[0][0] = 256;
    rgb2rgb1.matrix[0][1] = 0;
    rgb2rgb1.matrix[0][2] = 0;

    rgb2rgb1.matrix[1][0] = 0;
    rgb2rgb1.matrix[1][1] = 256;
    rgb2rgb1.matrix[1][2] = 0;

    rgb2rgb1.matrix[2][0] = 0;
    rgb2rgb1.matrix[2][1] = 0;
    rgb2rgb1.matrix[2][2] = 256;

    rgb2rgb1.offset[0]    = 0;
    rgb2rgb1.offset[1]    = 0;
    rgb2rgb1.offset[2]    = 0;


    rgb2rgb2.matrix[0][0] = 256;
    rgb2rgb2.matrix[0][1] = 0;
    rgb2rgb2.matrix[0][2] = 0;

    rgb2rgb2.matrix[1][0] = 0;
    rgb2rgb2.matrix[1][1] = 256;
    rgb2rgb2.matrix[1][2] = 0;

    rgb2rgb2.matrix[2][0] = 0;
    rgb2rgb2.matrix[2][1] = 0;
    rgb2rgb2.matrix[2][2] = 256;
    /*
    rgb2rgb2.matrix[0][0] = 360;
    rgb2rgb2.matrix[0][1] = -153;
    rgb2rgb2.matrix[0][2] = 49;

    rgb2rgb2.matrix[1][0] = -92;
    rgb2rgb2.matrix[1][1] = 312;
    rgb2rgb2.matrix[1][2] = 36;

    rgb2rgb2.matrix[2][0] = 37;
    rgb2rgb2.matrix[2][1] = -338;
    rgb2rgb2.matrix[2][2] = 557;
    */
    rgb2rgb2.offset[0]    = 0;
    rgb2rgb2.offset[1]    = 0;
    rgb2rgb2.offset[2]    = 0;

    if(DRV_ipipeSetRgb2Rgb(&rgb2rgb1) != CSL_SOK)
        OSA_ERROR("Fail DRV_ipipeSetRgb2Rgb2!!!\n");
    if(DRV_ipipeSetRgb2Rgb2(&rgb2rgb2) != CSL_SOK)
        OSA_ERROR("Fail DRV_ipipeSetRgb2Rgb2!!!\n");


    //Config isif white balance gain
    //DRV_isifSetDgain(512, 512, 512, 512, 0);
    DRV_isifSetDgain(hn->RGBgain[1], hn->RGBgain[0], hn->RGBgain[2], hn->RGBgain[1], 0);


    //Config ipipe gains and offset

    ipipeWb.gainR  = hn->GIFIF.New;
    ipipeWb.gainGr = hn->GIFIF.New;
    ipipeWb.gainGb = hn->GIFIF.New;
    ipipeWb.gainB  = hn->GIFIF.New;

    DRV_ipipeSetWbOffset(0);
    DRV_ipipeSetWb(&ipipeWb);
}

void SIG2A_applySettings(void)
{
    IAEWBF_SIG_Obj *hn = (IAEWBF_SIG_Obj *)gSIG_Obj.handle_aewbf;
    CSL_IpipeGammaConfig dataG;
    DRV_IpipeWb ipipeWb;
    CSL_IpipeRgb2RgbConfig rgb2rgb;

    OSA_printf("SIG2A_apply: gAePriorityMode = %d gBWMode = %d gDayNight =%d gIRCut = %d hn->gIRCut = %d defaultFPS = %d IRcutClose = %d hn->IRcutClose = %d FPShigh = %d hn->FPShigh = %d\n",
               gAePriorityMode, gBWMode, gDayNight, gIRCut, hn->gIRCut, defaultFPS, IRcutClose, hn->IRcutClose, FPShigh, hn->FPShigh);

    //IR-cut dynamic change
    if(gIRCut != hn->gIRCut) {
        if      (gIRCut == ALG_IRCUT_OPEN)  IRcutClose = 0;  // Open
        else if (gIRCut == ALG_IRCUT_CLOSE) IRcutClose = 1;  // Close
        hn->gIRCut = gIRCut;
    }

    //Black and color night mode dynamic change
    if(gBWMode != hn->gBWMode) {
        if (gIRCut == ALG_IRCUT_AUTO || gIRCut == ALG_IRCUT_OPEN) {
            DRV_imgsNDShutter(gIRCut, gBWMode);
            //DRV_imgsSetBWmode(gBWMode);
        }
        hn->gBWMode = gBWMode;
    }

    //DRV_imgsSetEshutter(33333, 0); //Max expouse
    //DRV_imgsSetFramerate(30); //Max FPS frame rate

    //Change FPS
    if (FPShigh != hn->FPShigh || gAePriorityMode != hn->gAePriorityMode) {
        if(FPShigh == 0 ){
            //Go to low FPS
            if (gAePriorityMode == ALG_FPS_LOW) {
                DRV_imgsSetFramerate(defaultFPS>>1);
                hn->Exp.Range.max = 1000000/(defaultFPS>>1);
                hn->Exp.New  = hn->Exp.Range.max;
            }
            else if (gAePriorityMode == ALG_FPS_5FPS){
                DRV_imgsSetFramerate(5);
                hn->Exp.Range.max = 200000;
                hn->Exp.New  = hn->Exp.Range.max;
            } else {
                DRV_imgsSetFramerate(defaultFPS);
                hn->Exp.Range.max = 1000000/defaultFPS;
                hn->Exp.New  = hn->Exp.Range.max;
            }
            //else if (gAePriorityMode == ALG_FPS_NONE)   DRV_imgsSetFramerate(defaultFPS);
        } else {
            //Go to high FPS
            DRV_imgsSetFramerate(defaultFPS);
            hn->Exp.Range.max = 1000000/defaultFPS;
            hn->Exp.New  = hn->Exp.Range.max;
        }
        hn->FPShigh = FPShigh;
        hn->gAePriorityMode = gAePriorityMode;
    }

    //Change IR-cut
    if (IRcutClose != hn->IRcutClose) {
        DRV_imgsNDShutter(IRcutClose, gBWMode);
        hn->IRcutClose = IRcutClose;
    }

    //Config gamma correction tables
    /*
   // if(hn->GISIF.New != hn->GISIF.Old || hn->Exp.New != hn->Exp.Old){
        dataG.tableSize = CSL_IPIPE_GAMMA_CORRECTION_TABLE_SIZE_512;
        dataG.tableSrc  = CSL_IPIPE_GAMMA_CORRECTION_TABLE_SELECT_RAM;
        dataG.bypassR = 0;
        dataG.bypassG = 0;
        dataG.bypassB = 0;
        dataG.tableR = hn->RGB[0].hist;
        dataG.tableG = hn->RGB[1].hist;
        dataG.tableB = hn->RGB[2].hist;
        //Setup gamma tables
        if(CSL_ipipeSetGammaConfig(&gCSL_ipipeHndl, &dataG) != CSL_SOK)
            OSA_ERROR("Fail CSL_ipipeSetGammaConfig!!!\n");
    //}
        */
    //Seting Expouse

    //DRV_imgsSetEshutter(33333, 0);
    if(hn->Exp.New != hn->Exp.Old) {
        DRV_imgsSetEshutter(hn->Exp.New, 0);
        hn->Exp.Old = hn->Exp.New;
    }

    //ISIF gain seting
    /*
    if(hn->GISIF.New != hn->GISIF.Old) {
        if(hn->RGBgain[0] != hn->GISIF.Range.max && hn->RGBgain[1] != hn->GISIF.Range.max && hn->RGBgain[2] != hn->GISIF.Range.max ){
            hn->RGBgain[1] = hn->GISIF.New;
            hn->RGBgain[0] = hn->GISIF.New*hn->RGBgain[0]/hn->GISIF.Old;
            hn->RGBgain[2] = hn->GISIF.New*hn->RGBgain[2]/hn->GISIF.Old;
        }
        hn->GISIF.Old = hn->GISIF.New;
        //OSA_printf("SIG2A_applySettings: new = %d old = %d Rgain = %d Ggain = %d Bgain = %d\n",
        //           hn->GISIF.New, hn->GISIF.Old, hn->RGBgain[0], hn->RGBgain[1], hn->RGBgain[2]);
    }*/
    DRV_isifSetDgain(hn->RGBgain[1] , hn->RGBgain[0], hn->RGBgain[2], hn->RGBgain[1], 0);

    if(hn->Offset.New != hn->Offset.Old) {
        DRV_ipipeSetWbOffset(-hn->Offset.New);
        hn->Offset.Old = hn->Offset.New;
    }

    //DRV_isifSetDgain(512, 512, 512, 512, 0);
    //gain = hn->Gain.New - hn->RGBgain[hn->maxi];
    //DRV_isifSetDgain(gain + hn->RGBgain[1] , gain + hn->RGBgain[0], gain + hn->RGBgain[2], gain + hn->RGBgain[1], 0);
    //gain = hn->Gain.New - hn->RGBgain[hn->maxi];


    //gain = (4000<<9)/(hn->Hmax[0] - hn->Hmin[0]);
    if(hn->GIFIF.New !=  hn->GIFIF.Old){
        hn->GIFIF.Old = hn->GIFIF.New;
        ipipeWb.gainR  = hn->GIFIF.New;
        ipipeWb.gainGr = hn->GIFIF.New;
        ipipeWb.gainGb = hn->GIFIF.New;
        ipipeWb.gainB  = hn->GIFIF.New;
        DRV_ipipeSetWb(&ipipeWb);
        hn->GIFIF.Old = hn->GIFIF.New;
    }

    //offset = hn->Hmin[0] > 2047 ? 2047 : hn->Hmin[0];
    //OSA_printf("SIG2A_applySettings: ofset = %d gain = %d\n", -offset, gain);
    //DRV_ipipeSetWbOffset(-offset);


    //Config RGB2RGB matrix
    //if(hn->Grgb2rgb.New !=  hn->Grgb2rgb.Old){
        rgb2rgb.matrix[0][0] = hn->Grgb2rgb.New; //hn->RGBgain[0]; //hn->Grgb2rgb.New;
        rgb2rgb.matrix[0][1] = 0;
        rgb2rgb.matrix[0][2] = 0;

        rgb2rgb.matrix[1][0] = 0;
        rgb2rgb.matrix[1][1] = hn->Grgb2rgb.New; //hn->RGBgain[1]; //hn->Grgb2rgb.New;
        rgb2rgb.matrix[1][2] = 0;

        rgb2rgb.matrix[2][0] = 0;
        rgb2rgb.matrix[2][1] = 0;
        rgb2rgb.matrix[2][2] = hn->Grgb2rgb.New; //hn->RGBgain[2]; //hn->Grgb2rgb.New;

        rgb2rgb.offset[0]    = 0;
        rgb2rgb.offset[1]    = 0;
        rgb2rgb.offset[2]    = 0;

        if(DRV_ipipeSetRgb2Rgb(&rgb2rgb) != CSL_SOK)
            OSA_ERROR("Fail DRV_ipipeSetRgb2Rgb2!!!\n");
        //if(DRV_ipipeSetRgb2Rgb2(&rgb2rgb) != CSL_SOK)
        //    OSA_ERROR("Fail DRV_ipipeSetRgb2Rgb2!!!\n");

        hn->Grgb2rgb.Old = hn->Grgb2rgb.New;
    //}
}


int SIG_2A_config(IALG_Handle handle)
{
    IAEWBF_SIG_Obj *hn = (IAEWBF_SIG_Obj *)handle;
    int i, stepSize, sensorExposureMax;

    i = 0; stepSize = 1;

    OSA_printf("SIG_2A_config: start\n");
    /*
    if (gSIG_Obj.sensorFps == 25) {
        ALG_aewbSetSensor50_60Hz(1); // 25FPS
    } else {
        ALG_aewbSetSensor50_60Hz(0); // 30FPS
    }
    */
    if (strcmp(DRV_imgsGetImagerName(), "MICRON_AR0331_1080P") == 0) 	// AR0331 sensor
    {
        if(gFlicker == VIDEO_NTSC)			// 60 Hz flicker
        {
            if (gSIG_Obj.sensorFps == 20)
            {
                stepSize = 6000; 	// Exposure stepsize
                //if (sensorExposureMax > 33333)
                sensorExposureMax = 48000;
            }
            else if (gSIG_Obj.sensorFps == 25){
                stepSize = 7500; 	// Exposure stepsize
                sensorExposureMax = 40000;
            } else if (gSIG_Obj.sensorFps == 30){
                stepSize = 9000; 	// Exposure stepsize
                sensorExposureMax = 33333;
            }
        } else if(gFlicker == VIDEO_PAL) { // 50 Hz flicker
            if (gSIG_Obj.sensorFps == 20)
            {
                stepSize = 7200; 	// Exposure stepsize
                //if (sensorExposureMax > 33333)
                sensorExposureMax = 43200;
            }
            else if (gSIG_Obj.sensorFps == 25) {
                stepSize = 9000; 	// Exposure stepsize
                sensorExposureMax = 40000;
            } else if (gSIG_Obj.sensorFps == 30) {
                stepSize = 10800; 	// Exposure stepsize
                sensorExposureMax = 33333;
            }
        } else {
            stepSize = 1;
        }
    } else if (strcmp(DRV_imgsGetImagerName(), "MICRON_MT9M034_720P") == 0) // MT9M034 sensor
    {
        if(gFlicker == VIDEO_NTSC)			// 60 Hz flicker
        {
            if (gSIG_Obj.sensorFps == 25){
                stepSize = 7084; 	// Exposure stepsize
                sensorExposureMax = 40000;
            } else if (gSIG_Obj.sensorFps == 30){
                stepSize = 8500; 	// Exposure stepsize
                sensorExposureMax = 33333;
            }
        } else if(gFlicker == VIDEO_PAL)	// 50 Hz flicker
        {
            if (gSIG_Obj.sensorFps == 25) {
                stepSize = 8500; 	// Exposure stepsize
                sensorExposureMax = 40000;
            } else if (gSIG_Obj.sensorFps == 30) {
                stepSize = 10200; 	// Exposure stepsize
                sensorExposureMax = 33333;
            }
        } else
        {
            stepSize = 1;
        }
    } else if (strcmp(DRV_imgsGetImagerName(), "MICRON_MT9P031_5MP") == 0) // MT9M031 sensor
    {
        if(gFlicker == VIDEO_NTSC)	{		// 60 Hz flicker
            stepSize = 8333; 	// Exposure stepsize
            sensorExposureMax = 33333;
        } else if(gFlicker == VIDEO_PAL) {	// 50 Hz flicker
            stepSize = 10000; 	// Exposure stepsize
            sensorExposureMax = 40000;
        } else {
            stepSize = 1;
        }
    } else if (strcmp(DRV_imgsGetImagerName(), "SONY_IMX136_3MP") == 0) // IMX136 sensor
    {
        if(gFlicker == VIDEO_NTSC) {		// 60 Hz flicker
            stepSize = 8333; 	// Exposure stepsize
            sensorExposureMax = 33333;
        } else if(gFlicker == VIDEO_PAL) {	// 50 Hz flicker
            stepSize = 10000; 	// Exposure stepsize
            sensorExposureMax = 40000;
        } else {
            stepSize = 1;
            sensorExposureMax = 1000000/defaultFPS;
        }
    } else
    {
        stepSize = 1;
    }
    /*
    DP.numRanges ++;

    DP.sensorGainRange[i].min = 1000;
    DP.sensorGainRange[i].max = 1000;
    DP.ipipeGainRange[i].min = 0;
    DP.ipipeGainRange[i].max = 8191;
    //DP.isifGainRange[i].min = 0;
    //DP.isifGainRange[i].max = 4095;
    DP.YRange.min = 0;
    DP.YRange.max = 0;
    */

    //DP.Ythresh = 10;
    //DP.ExpStep = stepSize;

    //sensorGain = 1000;
    //lowlight = DRV_imgsGetAEPriority();

    //Exposure setup
    hn->Exp.Step = stepSize;
    hn->Exp.Old = sensorExposureMax-1;
    hn->Exp.New = sensorExposureMax;
    hn->Exp.Max = sensorExposureMax;
    hn->Exp.Min = sensorExposureMax;
    hn->Exp.Range.max = sensorExposureMax;
    hn->Exp.Range.min = stepSize;
    hn->Exp.Th = 10; //10%
    hn->Exp.Diff = 0;

    printf("SIG_2A_config: hn->Exp.New = %d hn->Exp.Old = %d sensorExposureMax = %d gFlicker = %d\n",
           hn->Exp.New, hn->Exp.Old, sensorExposureMax, gFlicker);

    //ISIF gain setup
    hn->GISIF.Step = 16;
    hn->GISIF.Old = 511;
    hn->GISIF.New = 512;
    hn->GISIF.Max = 512;
    hn->GISIF.Min = 512;
    hn->GISIF.Range.min = 100;
    hn->GISIF.Range.max = 4095;
    hn->GISIF.Th = 10; //10%
    hn->GISIF.Diff = 0;

    //IFIF gain setup
    hn->GIFIF.Step = 16;
    hn->GIFIF.Old = 511;
    hn->GIFIF.New = 512;
    hn->GIFIF.Max = 512;
    hn->GIFIF.Min = 512;
    hn->GIFIF.Range.min = 512;
    hn->GIFIF.Range.max = 8180;
    hn->GIFIF.Th = 10; //10%
    hn->GIFIF.Diff = 0;

    //ISIF offset setup
    hn->Offset.Step = 1;
    hn->Offset.Old = 0;
    hn->Offset.New = 0;
    hn->Offset.Max = 0;
    hn->Offset.Min = 2000;
    hn->Offset.Range.min = 1;
    hn->Offset.Range.max = 4095;
    hn->Offset.Th = 10; //10%
    hn->Offset.Diff = 0;

    hn->Grgb2rgb.Old = 256;
    hn->Grgb2rgb.New = 256;
    hn->Grgb2rgb.Range.min = 1;
    hn->Grgb2rgb.Range.max = 2047;


    //Y setup
    hn->Y.Step = 1;
    hn->Y.New = 1;
    hn->Y.Old = 1;
    hn->Y.Max = 1;
    hn->Y.Min = 4095;
    hn->Y.Range.min = 0;
    hn->Y.Range.max = 4095;
    hn->Y.Th = 10; //10%
    hn->Y.Diff = 0;

    //Hmin setup
    hn->Hmin.New = 0;
    hn->Hmin.Old = 0;

    //Hmax setup
    hn->Hmax.New = 0;
    hn->Hmax.Old = 0;

    hn->RGBgain[0] = 512;
    hn->RGBgain[1] = 512;
    hn->RGBgain[2] = 512;

    hn->HhalfTh = 100;
    hn->Hhalf = 0;

    hn->HmaxTh = 3500;
    hn->RGB[0].MaxTh = hn->HmaxTh;
    hn->RGB[1].MaxTh = hn->HmaxTh;
    hn->RGB[2].MaxTh = hn->HmaxTh;
    hn->HminTh = 0;
    hn->SatTh = hn->w*hn->h*3/50;


    //First value of dymanic parameters
    hn->gAePriorityMode = gAePriorityMode;
    hn->gIRCut = gIRCut;
    hn->gBWMode = gBWMode;
    hn->IRcutClose = IRcutClose; //IR-cut 1-open, 0 - close
    hn->FPShigh = FPShigh; //FPS 1-high, 0 - low
    /*
    retval = IAEWBF_SIG.control((IAEWBF_Handle)gSIG_Obj.handle_aewbf, IAEWBF_CMD_SET_CONFIG, &DP, NULL);
    if(retval == -1) {
        OSA_ERROR("IAEWBF_SIG.control\n");
        return retval;
    }
    */
    ALG_SIG_config(gSIG_Obj.handle_aewbf);

    return 0;
}
