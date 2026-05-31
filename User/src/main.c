#include "inc/ti_msp_dl_config.h"
#include "inc/SPIflash.h"
#include "inc/OLED.h"
#include "inc/Alerts.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPIF_DEBUG SPIF_DEBUG_FULL

#define SPIF_TEST 0
#define AUDIO_PCM_DEMO 1
#define DAC_SINE_TEST 0
// Set to 1 when using CH341 to program external SPI flash in-circuit.
#define MCU_RELEASE_FLASH_BUS 0
#define AUDIO_PROGRAM_FLASH_ON_BOOT 0
#define AUDIO_EXTERNAL_FLASH_BYTES 5440000U
#define AUDIO_USE_DMA 0
#define AUDIO_DAC_BOOT_TONE 0
#define AUDIO_STABLE_STREAM 1
#define AUDIO_SAMPLE_RATE_HZ 8000U
#define AUDIO_RING_SAMPLES 8192U
#define AUDIO_OUTPUT_ATTENUATION_SHIFT 1U
/*
 * Blocking playback reads SPI flash in chunks and introduces short pauses,
 * which lowers average sample rate. Compensate by setting a slightly higher
 * in-chunk playback rate and tune by ear.
 */
#define AUDIO_BLOCKING_COMP_RATE_HZ 9000U
#define AUDIO_BLOCKING_SAMPLE_DELAY_CYCLES (CPUCLK_FREQ / AUDIO_BLOCKING_COMP_RATE_HZ)
#define AUDIO_PCM_FLASH_ADDR 0x00000000U
#define SPIF_TEST_DELAY_CYCLES 20000000U

#define KEY_PRESSED     false
#define KEY_RELEASED    true
#define STABLE_CNT      10
#define AUDIO_CHUNK_BYTES 1024U

#define WELCOME_ADDR    0x00
#define WELCOME_LEN     320
#define DIFF_ADDR0      0x10
#define DIFF_LEN        32
#define START_ADDR      0x20
#define START_LEN       64
#define LEVEL_ADDR0     0x30
#define LEVEL_LEN       64
#define BGM_ADDR        0x40
#define BGM_LEN         3200
#define WIN_ADDR        0x50
#define WIN_LEN         32
#define LOSE_ADDR       0x60
#define LOSE_LEN        32
#define WIN_ALL_ADDR    0x70
#define WIN_ALL_LEN     64

enum{
    WELCOME,
    DIFF_SEL,
    LEVEL_SEL,
    LEVEL_SEL_BUSY,
    PLAYING,
    GAME_OVER
}game_state;

enum{
    IDLE_BUF1,
    FILL_BUF1,
    IDLE_BUF2,
    FILL_BUF2
}buffer_state;

enum{
    IDLE,
    BUSY,
    SELECTED,
    CONFIRMED
}diff_sel_state;

enum{
    HIT,
    MISS
}key_result;

GPIO_Regs* LEDs_PORT[7] = {LEDs_LED1_PORT, LEDs_LED2_PORT, LEDs_LED3_PORT, LEDs_LED4_PORT, LEDs_LED5_PORT, LEDs_LED6_PORT, LEDs_LED7_PORT};
const uint32_t LEDs_PIN[7] = {LEDs_LED1_PIN, LEDs_LED2_PIN, LEDs_LED3_PIN, LEDs_LED4_PIN, LEDs_LED5_PIN, LEDs_LED6_PIN, LEDs_LED7_PIN};
GPIO_Regs* KEYs_PORT[7] = {KEYs_KEY1_PORT, KEYs_KEY2_PORT, KEYs_KEY3_PORT, KEYs_KEY4_PORT, KEYs_KEY5_PORT, KEYs_KEY6_PORT, KEYs_KEY7_PORT};
const uint32_t KEYs_PIN[7] = {KEYs_KEY1_PIN, KEYs_KEY2_PIN, KEYs_KEY3_PIN, KEYs_KEY4_PIN, KEYs_KEY5_PIN, KEYs_KEY6_PIN, KEYs_KEY7_PIN};

const uint16_t SPEED[7] = {65535, 57343, 49151, 40959, 32767, 24575, 16383};
const uint16_t TARGET_SCORE[7][3] = {{20, 50, 90}, {40, 100, 160}, {50, 150, 240}, {80, 250, 320}, {100, 280, 390}, {120, 300, 460}, {150, 300, 540}};

bool        key_state[7];
volatile bool is_key_triggered[7];
volatile bool is_LED_active;
uint8_t     key_stable_cnt[7];
uint8_t     difficulty,level;
volatile uint8_t target1,target2,game_prog;
uint16_t    buffer1[512],buffer2[512];
uint16_t    play_prog1,play_prog2,score;

static SPIF_HandleTypeDef gSpifHandle;
SPIF_HandleTypeDef *Handle = &gSpifHandle;
static uint32_t gAudioChunkCount = 0;
static bool gSpifReadFail = false;
static volatile bool gAudioUnderflow = false;
static volatile uint16_t gAudioRing[AUDIO_RING_SAMPLES];
static volatile uint32_t gAudioRingRead = 0;
static volatile uint32_t gAudioRingWrite = 0;

static void DacBootToneTest(void)
{
#if AUDIO_DAC_BOOT_TONE
    // 2s square-wave check with direct DATA0 write (bypasses FIFO trigger chain).
    for (uint32_t n = 0; n < 2000U; n++)
    {
        DAC0->DATA0 = 4095U;
        DL_Common_delayCycles(16000);
        DAC0->DATA0 = 0U;
        DL_Common_delayCycles(16000);
    }
#endif
}

#if AUDIO_PCM_DEMO && AUDIO_PROGRAM_FLASH_ON_BOOT
#include "inc/audio_pcm.h"
#endif

#if SPIF_TEST
static void SPIF_Test(void)
{
    uint8_t tx[256];
    uint8_t rx[256];
    uint8_t manuf = 0;
    uint8_t mem_type = 0;
    uint8_t capacity = 0;
    uint8_t status1 = 0;
    uint8_t status1_after_wel = 0;
    uint8_t bp_bits = 0;
    uint16_t mismatch_index = 0xFFFF;
    bool ok = true;

    OLED_Clear();
    if (SPIF_Init(Handle) == false)
    {
        OLED_ShowString(1, 1, "SPIF INIT");
        OLED_ShowString(2, 1, "FAIL");
        return;
    }

    if (SPIF_ReleasePowerDown(Handle) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "WAKE");
        OLED_ShowString(2, 1, "FAIL");
        return;
    }

    if (SPIF_ReadJedecId(Handle, &manuf, &mem_type, &capacity) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "JEDEC");
        OLED_ShowString(2, 1, "READ FAIL");
        return;
    }
    OLED_Clear();
    OLED_ShowString(1, 1, "M:");
    OLED_ShowHexNum(1, 3, manuf, 2);
    OLED_ShowString(1, 6, "T:");
    OLED_ShowHexNum(1, 8, mem_type, 2);
    OLED_ShowString(2, 1, "C:");
    OLED_ShowHexNum(2, 3, capacity, 2);
    delay_cycles(SPIF_TEST_DELAY_CYCLES);

    status1 = SPIF_ReadStatus1(Handle);
    bp_bits = (status1 >> 2) & 0x7U;
    OLED_Clear();
    OLED_ShowString(1, 1, "S1:");
    OLED_ShowHexNum(1, 4, status1, 2);
    OLED_ShowString(1, 8, "BP:");
    OLED_ShowHexNum(1, 11, bp_bits, 1);
    OLED_ShowString(2, 1, "B:");
    OLED_ShowHexNum(2, 3, status1 & 0x1U, 1);
    OLED_ShowString(2, 6, "W:");
    OLED_ShowHexNum(2, 8, (status1 >> 1) & 0x1U, 1);
    delay_cycles(SPIF_TEST_DELAY_CYCLES);

    if (bp_bits != 0)
    {
        if (SPIF_WriteStatus1(Handle, 0x00) == false)
        {
            OLED_Clear();
            OLED_ShowString(1, 1, "CLR BP");
            OLED_ShowString(2, 1, "FAIL");
            return;
        }
        delay_cycles(SPIF_TEST_DELAY_CYCLES);
        status1 = SPIF_ReadStatus1(Handle);
        bp_bits = (status1 >> 2) & 0x7U;
        OLED_Clear();
        OLED_ShowString(1, 1, "S1:");
        OLED_ShowHexNum(1, 4, status1, 2);
        OLED_ShowString(1, 8, "BP:");
        OLED_ShowHexNum(1, 11, bp_bits, 1);
        OLED_ShowString(2, 1, "CLR");
        OLED_ShowString(2, 5, "DONE");
        delay_cycles(SPIF_TEST_DELAY_CYCLES);
    }

    if (SPIF_SetWriteEnable(Handle) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "WREN");
        OLED_ShowString(2, 1, "FAIL");
        return;
    }
    status1_after_wel = SPIF_ReadStatus1(Handle);
    OLED_Clear();
    OLED_ShowString(1, 1, "S1:");
    OLED_ShowHexNum(1, 4, status1_after_wel, 2);
    OLED_ShowString(2, 1, "WEL:");
    OLED_ShowHexNum(2, 5, (status1_after_wel >> 1) & 0x1U, 1);
    delay_cycles(SPIF_TEST_DELAY_CYCLES);

    OLED_Clear();
    OLED_ShowString(1, 1, "ERASE");
    OLED_ShowString(2, 1, "START");

    for (uint16_t i = 0; i < sizeof(tx); i++)
    {
        tx[i] = (uint8_t)(i ^ 0xA5U);
        rx[i] = 0;
    }

    if (SPIF_EraseSector(Handle, 0) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "ERASE");
        OLED_ShowString(2, 1, "TIMEOUT");
        return;
    }
    OLED_Clear();
    OLED_ShowString(1, 1, "ERASE");
    OLED_ShowString(2, 1, "OK");
    delay_cycles(SPIF_TEST_DELAY_CYCLES);

    OLED_Clear();
    OLED_ShowString(1, 1, "WRITE");
    OLED_ShowString(2, 1, "START");
    if (SPIF_WriteAddress(Handle, 0, tx, sizeof(tx)) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "WRITE");
        OLED_ShowString(2, 1, "FAIL");
        return;
    }
    OLED_Clear();
    OLED_ShowString(1, 1, "WRITE");
    OLED_ShowString(2, 1, "OK");
    delay_cycles(SPIF_TEST_DELAY_CYCLES);

    OLED_Clear();
    OLED_ShowString(1, 1, "READ");
    OLED_ShowString(2, 1, "START");
    if (SPIF_ReadAddress(Handle, 0, rx, sizeof(rx)) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "READ");
        OLED_ShowString(2, 1, "FAIL");
        return;
    }
    OLED_Clear();
    OLED_ShowString(1, 1, "READ");
    OLED_ShowString(2, 1, "OK");
    delay_cycles(SPIF_TEST_DELAY_CYCLES);

    for (uint16_t i = 0; i < sizeof(tx); i++)
    {
        if (rx[i] != tx[i])
        {
            mismatch_index = i;
            ok = false;
            break;
        }
    }

    OLED_Clear();
    if (ok)
    {
        OLED_ShowString(1, 1, "SPIF OK");
        OLED_ShowString(2, 1, "RD/WR OK");
    }
    else
    {
        OLED_ShowString(1, 1, "SPIF FAIL");
        OLED_ShowString(2, 1, "I:");
        OLED_ShowHexNum(2, 3, mismatch_index, 4);
    }
}
#endif

#if DAC_SINE_TEST
static void Dac_SineTest(void)
{
    // 显示 DAC 放大器、FIFO、输出引脚和采样率以便调试
    DL_DAC12_AMP amp = DL_DAC12_getAmplifier(DAC0);
    bool fifo_en = DL_DAC12_isFIFOEnabled(DAC0);
    bool out_en = DL_DAC12_isOutputPinEnabled(DAC0);
    DL_DAC12_SAMPLES_PER_SECOND sps = DL_DAC12_getSampleRate(DAC0);
    DL_DAC12_FIFO_THRESHOLD thr = DL_DAC12_getFIFOThreshold(DAC0);

    OLED_Clear();
    OLED_ShowString(1, 1, "DAC DEBUG");
    OLED_ShowString(2, 1, "AMP:");
    OLED_ShowHexNum(2, 6, (uint32_t)amp, 2);
    OLED_ShowString(3, 1, "FIFO:");
    OLED_ShowString(3, 7, fifo_en ? "ON" : "OFF");
    OLED_ShowString(4, 1, "OUT:");
    OLED_ShowString(4, 6, out_en ? "ON" : "OFF");
    OLED_ShowString(5, 1, "SPS:");
    OLED_ShowHexNum(5, 6, (uint32_t)sps, 2);
    OLED_ShowString(6, 1, "TH:");
    OLED_ShowHexNum(6, 5, (uint32_t)thr, 2);

    // 输出强力方波（在 LM386 前端用耦合电容会更容易听到）
    while (1)
    {
        // 高 -> 4095
        DL_DAC12_outputBlocking12(DAC0, 4095);
        DL_Common_delayCycles(20000);
        // 低 -> 0
        DL_DAC12_outputBlocking12(DAC0, 0);
        DL_Common_delayCycles(20000);
    }
}
#endif

static void LoadNextAudioChunk(uint16_t *dst)
{
    int16_t raw[AUDIO_CHUNK_BYTES / 2U];

#if AUDIO_PCM_DEMO
    if (gAudioChunkCount != 0 && play_prog1 >= gAudioChunkCount)
    {
        play_prog1 = 0;
    }
#endif
    uint32_t addr = ((uint32_t)play_prog1++) * AUDIO_CHUNK_BYTES;
    if (SPIF_ReadAddress(Handle, addr, (uint8_t *)raw, AUDIO_CHUNK_BYTES) == false)
    {
        gSpifReadFail = true;
        for (uint32_t i = 0; i < (AUDIO_CHUNK_BYTES / 2U); i++)
        {
            dst[i] = 2048U;
        }
        return;
    }

    // Convert signed 16-bit PCM to unsigned 12-bit DAC samples.
    // Use unbiased mapping: int16_t (-32768..32767) -> uint32 0..65535 -> 0..4095
    // Reserve a small headroom (2%) to avoid clipping at the amplifier.
    for (uint32_t i = 0; i < (AUDIO_CHUNK_BYTES / 2U); i++)
    {
        int32_t s = (int32_t)raw[i];
        uint32_t u = (uint32_t)(s + 32768); // 0 .. 65535
        uint32_t dac = (uint32_t)((u * 4095ULL) / 65535ULL);
        // Apply small attenuation to provide headroom (98%)
        dac = (dac * 98U) / 100U;
        if (dac > 4095U) dac = 4095U;
        dst[i] = (uint16_t)dac;
    }
}

#if AUDIO_PCM_DEMO && AUDIO_PROGRAM_FLASH_ON_BOOT
static bool ProgramAudioPcmToFlash(void)
{
    uint32_t total = audio_pcm_len_bytes;
    uint32_t start_sector = SPIF_AddressToSector(AUDIO_PCM_FLASH_ADDR);
    uint32_t sector_count = (total + SPIF_SECTOR_SIZE - 1U) / SPIF_SECTOR_SIZE;

    if (SPIF_Init(Handle) == false)
    {
        return false;
    }

    for (uint32_t i = 0; i < sector_count; i++)
    {
        if (SPIF_EraseSector(Handle, start_sector + i) == false)
        {
            return false;
        }
    }

    if (SPIF_WriteAddress(Handle, AUDIO_PCM_FLASH_ADDR, (uint8_t *)audio_pcm, total) == false)
    {
        return false;
    }

    return true;
}
#endif

void StartDMA(){
    LoadNextAudioChunk(buffer1);
    LoadNextAudioChunk(buffer2);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
}

static uint32_t AudioRingFree(void)
{
    uint32_t r = gAudioRingRead;
    uint32_t w = gAudioRingWrite;
    if (w >= r)
    {
        return (AUDIO_RING_SAMPLES - (w - r) - 1U);
    }
    return (r - w - 1U);
}

static void AudioRingPushChunk(const uint16_t *src, uint32_t count)
{
    uint32_t w = gAudioRingWrite;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t next = (w + 1U) % AUDIO_RING_SAMPLES;
        if (next == gAudioRingRead)
        {
            break;
        }
        gAudioRing[w] = src[i];
        w = next;
    }
    gAudioRingWrite = w;
}

static void FillAudioRingFromFlash(void)
{
    while (AudioRingFree() >= ((AUDIO_CHUNK_BYTES / 2U) + 1U))
    {
        LoadNextAudioChunk(buffer1);
        AudioRingPushChunk(buffer1, (AUDIO_CHUNK_BYTES / 2U));
    }
}

void SysTick_Handler(void)
{
    uint16_t sample = 2048U;
    uint32_t r = gAudioRingRead;
    if (r != gAudioRingWrite)
    {
        sample = gAudioRing[r];
        gAudioRingRead = (r + 1U) % AUDIO_RING_SAMPLES;
    }
    else
    {
        gAudioUnderflow = true;
    }
    DAC0->DATA0 = sample;
}



void FillBuffer(){
    if (buffer_state == FILL_BUF1){
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer2[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        LoadNextAudioChunk(buffer1);
        for (uint8_t i = 0; i < 7; i++){
            if ((i == target1 || i == target2) && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = HIT;
                is_key_triggered[i] = false;
                DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
                if (i == target1)
                    target1 = 8;
                else
                    target2 = 8;
                score += 3;
                OLED_ShowNum(3, 10, score, 3);
            }
            if (i != target1 && i != target2 && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = MISS;
                is_key_triggered[i] = false;
                if (score > 0)
                    score -= 1;
                OLED_ShowNum(3, 10, score, 3);
            }
        }
        if (play_prog2){
            for (uint16_t i = 0; i < 512; i++){
                if (key_result == HIT)
                    buffer1[i] = alert_hit[play_prog2++];
                else
                    buffer1[i] = alert_miss[play_prog2++];
                if (play_prog2 >= ALERT_LEN){
                    play_prog2 = 0;
                    break;
                }
            }
            
        }
        buffer_state = IDLE_BUF1;
    }
    else if (buffer_state == FILL_BUF2){
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        LoadNextAudioChunk(buffer2);
        for (uint8_t i = 0; i < 7; i++){
            if ((i == target1 || i == target2) && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = HIT;
                is_key_triggered[i] = false;
                DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
                if (i == target1)
                    target1 = 8;
                else
                    target2 = 8;
                score += 3;
                OLED_ShowNum(3, 10, score, 3);
            }
            if (i != target1 && i != target2 && is_key_triggered[i]){
                play_prog2 = 1;
                key_result = MISS;
                is_key_triggered[i] = false;
                if (score > 0)
                    score -= 1;
                OLED_ShowNum(3, 10, score, 3);
            }
        }
        if (play_prog2){
            for (uint16_t i = 0; i < 512; i++){
                if (key_result == HIT)
                    buffer2[i] = alert_hit[play_prog2++];
                else
                    buffer2[i] = alert_miss[play_prog2++];
                if (play_prog2 >= ALERT_LEN){
                    play_prog2 = 0;
                    break;
                }
            }
            
        }
        buffer_state = IDLE_BUF2;
    }
}

#if MCU_RELEASE_FLASH_BUS
static void ReleaseFlashBusForExternalProgrammer(void)
{
    // Put flash bus pins in Hi-Z so external programmer can drive the bus.
    DL_GPIO_initDigitalInputFeatures(COMs_CS_Flash_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(COMs_SCLK_Flash_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(COMs_PICO_Flash_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(COMs_POCI_Flash_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_disableOutput(COMs_CS_Flash_PORT, COMs_CS_Flash_PIN);
    DL_GPIO_disableOutput(COMs_SCLK_Flash_PORT, COMs_SCLK_Flash_PIN);
    DL_GPIO_disableOutput(COMs_PICO_Flash_PORT, COMs_PICO_Flash_PIN);
    DL_GPIO_disableOutput(COMs_POCI_Flash_PORT, COMs_POCI_Flash_PIN);
}
#endif

int main(void) {
    SYSCFG_DL_init();
    //SPI_Flash_SetBitrate10kHz();
    //SPIF_Init(Handle);
#if !AUDIO_PCM_DEMO
    __NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
    __NVIC_EnableIRQ(DAC12_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_KEYs_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_KEYs_INST_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_LEDs_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_LEDs_INST_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_PROG_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_PROG_INST_INT_IRQN);
    DL_Timer_startCounter(TIMER_KEYs_INST);
    DL_Timer_startCounter(TIMER_LEDs_INST);
    DL_Timer_startCounter(TIMER_PROG_INST);
#endif
    OLED_Init();
#if MCU_RELEASE_FLASH_BUS
    ReleaseFlashBusForExternalProgrammer();
    OLED_Clear();
    OLED_ShowString(1, 1, "FLASH BUS");
    OLED_ShowString(2, 1, "RELEASED");
    while (1) {
    }
#endif
#if SPIF_TEST
    SPIF_Test();
    while (1) {
    }
#endif
#if DAC_SINE_TEST
    Dac_SineTest();
#endif
#if AUDIO_PCM_DEMO
#if AUDIO_PROGRAM_FLASH_ON_BOOT
    if (ProgramAudioPcmToFlash() == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "PCM WR");
        OLED_ShowString(2, 1, "FAIL");
        while (1) {
        }
    }
#endif
    if (SPIF_Init(Handle) == false)
    {
        OLED_Clear();
        OLED_ShowString(1, 1, "SPIF INIT");
        OLED_ShowString(2, 1, "FAIL");
        while (1) {
        }
    }

#if AUDIO_PROGRAM_FLASH_ON_BOOT
    gAudioChunkCount = (audio_pcm_len_bytes + AUDIO_CHUNK_BYTES - 1U) / AUDIO_CHUNK_BYTES;
#else
    gAudioChunkCount = (AUDIO_EXTERNAL_FLASH_BYTES + AUDIO_CHUNK_BYTES - 1U) / AUDIO_CHUNK_BYTES;
#endif
    play_prog1 = 0;
    buffer_state = IDLE_BUF2;
#if AUDIO_USE_DMA
    StartDMA();
    while (1)
    {
        FillBuffer();
    }
#else
    OLED_Clear();
    OLED_ShowString(1, 1, "AUDIO PLAY");
    OLED_ShowString(2, 1, "BOOT TONE");
    DL_DAC12_disableFIFO(DAC0);
    DacBootToneTest();
    OLED_ShowString(2, 1, "PLAY PCM ");
    DL_Timer_stopCounter(TIMER_KEYs_INST);
    DL_Timer_stopCounter(TIMER_LEDs_INST);
    DL_Timer_stopCounter(TIMER_PROG_INST);
    gAudioRingRead = 0;
    gAudioRingWrite = 0;
    gAudioUnderflow = false;
    FillAudioRingFromFlash();
    if (SysTick_Config(CPUCLK_FREQ / AUDIO_SAMPLE_RATE_HZ) != 0U)
    {
        OLED_ShowString(3, 1, "SYSTICK FAIL");
        while (1) {
        }
    }
    while (1)
    {
        FillAudioRingFromFlash();
        if (gSpifReadFail)
        {
            OLED_ShowString(2, 1, "SPIF READ FAIL");
            gSpifReadFail = false;
        }
        if (gAudioUnderflow)
        {
            OLED_ShowString(3, 1, "AUDIO UNDERFLOW");
            gAudioUnderflow = false;
        }
    }
#endif
#endif
    OLED_ShowCoverIMG();
    delay_cycles(50000000);
    OLED_Clear();
    OLED_ShowString(1, 1, "Hello, World!");
    OLED_ShowChinese(2, 1, 0);
    uint8_t i=0;
    while (1) {
        OLED_ShowChinese(2, 2, 1);
        DL_GPIO_togglePins(COMs_CS_Flash_PORT, COMs_CS_Flash_PIN);
        // DL_GPIO_togglePins(GPIO_SPI_Flash_PICO_PORT, GPIO_SPI_Flash_PICO_PIN);
        // DL_GPIO_togglePins(GPIO_SPI_Flash_POCI_PORT, GPIO_SPI_Flash_POCI_PIN);
        // DL_GPIO_togglePins(GPIO_SPI_Flash_SCLK_PORT, GPIO_SPI_Flash_SCLK_PIN);
        OLED_ShowNum(2, 6, (i++) % 10, 1);
        delay_cycles(20000000);
    }
}

// int main(void) {
//     SYSCFG_DL_init();
//     __NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
//     __NVIC_EnableIRQ(DAC12_INT_IRQN);
//     __NVIC_ClearPendingIRQ(TIMER_KEYs_INST_INT_IRQN);
//     __NVIC_EnableIRQ(TIMER_KEYs_INST_INT_IRQN);
//     __NVIC_ClearPendingIRQ(TIMER_LEDs_INST_INT_IRQN);
//     __NVIC_EnableIRQ(TIMER_LEDs_INST_INT_IRQN);
//     __NVIC_ClearPendingIRQ(TIMER_PROG_INST_INT_IRQN);
//     __NVIC_EnableIRQ(TIMER_PROG_INST_INT_IRQN);
//     SPIF_Init(Handle);

//     for (uint8_t i = 0; i < 7; i++){
//         key_state[i] = KEY_RELEASED;
//         is_key_triggered[i] = false;
//         key_stable_cnt[i] = 0;
//     }
//     is_LED_active = false;
//     game_state = WELCOME;
//     buffer_state = IDLE_BUF2;
//     diff_sel_state = IDLE;
//     key_result = HIT;
//     difficulty = 0;
//     level = 0;
//     target1 = 8;
//     target2 = 8;
//     game_prog = 0;
//     play_prog1 = WELCOME_ADDR;
//     play_prog2 = 0;
//     StartDMA();
//     OLED_ShowCoverIMG();

//     while (1) {
//         switch (game_state){
//             //欢迎界面
//             case WELCOME:
//                 while (play_prog1 < WELCOME_ADDR + WELCOME_LEN)
//                     FillBuffer();
//                 DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                 DL_Timer_startCounter(TIMER_KEYs_INST);
//                 OLED_Clear();
//                 OLED_ShowChinese(2, 2, 0);
//                 OLED_ShowChinese(2, 4, 1);
//                 for (uint8_t i = 0; i < 7; i++)
//                     OLED_ShowChinese(4, i + 2, i + 4);
//                 game_state = DIFF_SEL;
//             break;

//             //难度选择
//             case DIFF_SEL:
//                 if (diff_sel_state == BUSY){
//                     while (play_prog1 < DIFF_ADDR0 + (difficulty + 1) * DIFF_LEN)
//                         FillBuffer();
//                     DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                     DL_Timer_startCounter(TIMER_KEYs_INST);
//                     diff_sel_state = SELECTED;
//                 }
//                 else if (diff_sel_state == CONFIRMED){
//                     while (play_prog1 < START_ADDR + START_LEN)
//                         FillBuffer();
//                     DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                     DL_Timer_setLoadValue(TIMER_LEDs_INST, SPEED[difficulty]);
//                     level = 0;
//                     score = 0;
//                     diff_sel_state = IDLE;
//                     game_state = LEVEL_SEL;
//                 }
//                 else {
//                     for (uint8_t i = 0; i < 7; i++){
//                         if (is_key_triggered[i]){
//                             is_key_triggered[i] = false;
//                             if (diff_sel_state == IDLE){
//                                 play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
//                                 difficulty = i;
//                                 DL_Timer_stopCounter(TIMER_KEYs_INST);
//                                 OLED_ShowNum(1, 10, i + 1, 1);
//                                 diff_sel_state = BUSY;
//                                 StartDMA();
//                             }
//                             else if (diff_sel_state == SELECTED){
//                                 if (i == difficulty){
//                                     play_prog1 = START_ADDR;
//                                     DL_Timer_stopCounter(TIMER_KEYs_INST);
//                                     diff_sel_state = CONFIRMED;
//                                     StartDMA();
//                                 }
//                                 else{
//                                     play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
//                                     difficulty = i;
//                                     DL_Timer_stopCounter(TIMER_KEYs_INST);
//                                     OLED_ShowNum(1, 10, i + 1, 1);
//                                     diff_sel_state = BUSY;
//                                     StartDMA();
//                                 }
//                             }
//                         }
//                     }
//                 }
//             break;

//             //关卡选择
//             case LEVEL_SEL:
//                 play_prog1 = LEVEL_ADDR0 + level * LEVEL_LEN;
//                 OLED_Clear();
//                 OLED_ShowChinese(1, 3, 2);
//                 OLED_ShowNum(1, 7, level + 1, 1);
//                 OLED_ShowChinese(1, 5, 3);

//                 OLED_ShowChinese(2, 2, 11);
//                 OLED_ShowChinese(2, 3, 12);
//                 OLED_ShowChar(2, 8, ':');
//                 OLED_ShowNum(2, 10, TARGET_SCORE[difficulty][level], 3);

//                 OLED_ShowChinese(3, 2, 13);
//                 OLED_ShowChinese(3, 3, 14);
//                 OLED_ShowChar(3, 8, ':');
//                 OLED_ShowNum(3, 10, score, 3);
//                 game_state = LEVEL_SEL_BUSY;
//                 StartDMA();
//             break;

//             //开始音乐播放
//             case LEVEL_SEL_BUSY:
//                 while (play_prog1 < LEVEL_ADDR0 + (level + 1) * LEVEL_LEN)
//                     FillBuffer();
//                 DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                 DL_Timer_startCounter(TIMER_KEYs_INST);
//                 game_state = PLAYING;
//                 play_prog1 = BGM_ADDR;
//                 srand((unsigned)time(NULL));
//                 DL_Timer_startCounter(TIMER_LEDs_INST);
//                 DL_Timer_startCounter(TIMER_PROG_INST);
//                 OLED_SetCursor(7, 0);
//                 StartDMA();
//             break;

//             //开始游戏
//             case PLAYING:
//                 if (play_prog1 >= BGM_ADDR + BGM_LEN)
//                     play_prog1 = BGM_ADDR;
//                 FillBuffer();
//                 if (game_prog >= 120){
//                     DL_Timer_stopCounter(TIMER_KEYs_INST);
//                     DL_Timer_stopCounter(TIMER_LEDs_INST);
//                     DL_Timer_stopCounter(TIMER_PROG_INST);
//                     game_prog = 0;
//                     for (uint8_t i = 0; i < 7; i++)
//                         DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
//                     target1 = 8;
//                     target2 = 8;
//                     game_state = GAME_OVER;
//                     DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                     StartDMA();
//                 }
//             break;

//             //游戏结束
//             case GAME_OVER:
//                 OLED_Clear();
//                 if (score >= TARGET_SCORE[difficulty][level]){
//                     if (level < 2){
//                         play_prog1 = WIN_ADDR;
//                         while (play_prog1 < WIN_ADDR + WIN_LEN)
//                             FillBuffer();
//                         DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                         level++;
//                         game_state = LEVEL_SEL;
//                     }
//                     else{
//                         play_prog1 = WIN_ALL_ADDR;
//                         while (play_prog1 < WIN_ALL_ADDR + WIN_ALL_LEN)
//                             FillBuffer();
//                         DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                         game_state = DIFF_SEL;
//                     }
//                 }
//                 else{
//                     play_prog1 = LOSE_ADDR;
//                     while (play_prog1 < LOSE_ADDR + LOSE_LEN)
//                         FillBuffer();
//                     DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
//                     game_state = DIFF_SEL;
//                 }
//             break;
//             default:break;
//         }
//     }
// }

// void DAC12_IRQHandler(){
//     switch (DAC12_INT_IRQN){
//         case DL_DAC12_IIDX_DMA_DONE:
//             if (buffer_state == IDLE_BUF1)
//                 buffer_state = FILL_BUF2;
//             else if (buffer_state == IDLE_BUF2)
//                 buffer_state = FILL_BUF1;
//         break;
//         default:break;
//     }
// }

void TIMER_KEYs_INST_IRQHandler(){
    switch (DL_Timer_getPendingInterrupt(TIMER_KEYs_INST)){
        case DL_TIMER_IIDX_ZERO:
        {
            bool current_state;
            for (uint8_t i = 0; i < 7; i++){
                current_state = DL_GPIO_readPins((GPIO_Regs *)KEYs_PORT[i], KEYs_PIN[i]);
                if (current_state == key_state[i])
                    key_stable_cnt[i] = 0;
                else{
                    key_stable_cnt[i]++;
                    if (key_stable_cnt[i] >= STABLE_CNT){
                        key_state[i] = current_state;
                        key_stable_cnt[i] = 0;
                        if (current_state == KEY_PRESSED){
                            is_key_triggered[i] = true;
                            DL_GPIO_togglePins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
                        }
                    }
                }
            }
        }

        break;
        default:break;
    }
}

void TIMER_LEDs_INST_IRQHandler(){
    switch (DL_Timer_getPendingInterrupt(TIMER_LEDs_INST)){
        case DL_TIMER_IIDX_ZERO:
            
            //DL_GPIO_togglePins((GPIO_Regs *)LEDs_PORT[1], LEDs_PIN[1]);
        
        // is_LED_active = !is_LED_active;
            // if (is_LED_active){
            //     target1 = rand() % 7;
            //     DL_GPIO_setPins((GPIO_Regs *)LEDs_PORT[target1], LEDs_PIN[target1]);
            //     if (difficulty >= 4){
            //         do{
            //             target2 = rand() % 7;
            //         }while(target2 == target1);
            //         DL_GPIO_setPins((GPIO_Regs *)LEDs_PORT[target2], LEDs_PIN[target2]);
            //     }
            //     else
            //         target2 = 8;
            // }
            // else{
            //     for (uint8_t i = 0; i < 7; i++)
            //         DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
            //     target1 = 8;
            //     target2 = 8;
            // }
        break;
        default:break;
    }
}

void TIMER_PROG_INST_IRQHandler(){
    switch (DL_Timer_getPendingInterrupt(TIMER_PROG_INST)){
        case DL_TIMER_IIDX_ZERO:
            
            //DL_GPIO_togglePins((GPIO_Regs *)LEDs_PORT[2], LEDs_PIN[2]);
        
            // game_prog++;
            // OLED_WriteData(0xFF);
        break;
        default:break;
    }
}