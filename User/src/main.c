#include "inc/ti_msp_dl_config.h"
#include "inc/SPIflash.h"
#include "inc/OLED.h"
#include "inc/Alerts.h"
#include <stdlib.h>
#include <string.h>

#define SPIF_DEBUG SPIF_DEBUG_FULL

#define SPIF_TEST 0
#define GAME_DEMO 1
#define AUDIO_PCM_DEMO 0
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

#define KEY_PRESSED     true
#define KEY_RELEASED    false
#define STABLE_CNT      10
#define AUDIO_CHUNK_BYTES 1024U
#define WELCOME_DURATION_TICKS 12U
#define GAME_DURATION_TICKS 240U
#define PROGRESS_PAGE 3U
#define PROGRESS_X 0U
#define PROGRESS_WIDTH 32U
#define PROGRESS_INNER_WIDTH (PROGRESS_WIDTH - 2U)
#define RESULT_FLASH_CHUNKS 8U

#define WELCOME_ADDR    0x000
#define WELCOME_LEN     48
#define DIFF_ADDR0      0x030
#define DIFF_LEN        16
#define START_ADDR      0x0A0
#define START_LEN       47
#define LEVEL_ADDR0     0x0CF
#define LEVEL_LEN       47
#define BGM_ADDR0       0x15C
#define BGM_LEN         938
#define WIN_ADDR        0xC5A
#define WIN_LEN         47
#define LOSE_ADDR       0xC89
#define LOSE_LEN        47
#define WIN_ALL_ADDR    0xCB8
#define WIN_ALL_LEN     47
#define HIDDEN_WIN_ADDR 0xCE7
#define HIDDEN_WIN_LEN  47

enum{
    WELCOME,
    DIFF_SEL,
    LEVEL_SEL,
    LEVEL_SEL_BUSY,
    PLAYING,
    GAME_OVER,
    GAME_FINISHED
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

typedef enum {
    HIT,
    MISS,
    NO_HIT
} KeyResult;

volatile KeyResult key_result;

GPIO_Regs* LEDs_PORT[7] = {LEDs_LED1_PORT, LEDs_LED2_PORT, LEDs_LED3_PORT, LEDs_LED4_PORT, LEDs_LED5_PORT, LEDs_LED6_PORT, LEDs_LED7_PORT};
const uint32_t LEDs_PIN[7] = {LEDs_LED1_PIN, LEDs_LED2_PIN, LEDs_LED3_PIN, LEDs_LED4_PIN, LEDs_LED5_PIN, LEDs_LED6_PIN, LEDs_LED7_PIN};
GPIO_Regs* KEYs_PORT[7] = {KEYs_KEY1_PORT, KEYs_KEY2_PORT, KEYs_KEY3_PORT, KEYs_KEY4_PORT, KEYs_KEY5_PORT, KEYs_KEY6_PORT, KEYs_KEY7_PORT};
const uint32_t KEYs_PIN[7] = {KEYs_KEY1_PIN, KEYs_KEY2_PIN, KEYs_KEY3_PIN, KEYs_KEY4_PIN, KEYs_KEY5_PIN, KEYs_KEY6_PIN, KEYs_KEY7_PIN};

const uint16_t SPEED[7] = {65535, 57343, 49151, 40959, 32767, 24575, 16383};
const uint16_t TARGET_SCORE[7][3] = {{20, 50, 90}, {40, 100, 160}, {50, 150, 240}, {80, 250, 320}, {100, 280, 390}, {120, 300, 460}, {150, 300, 540}};

bool        key_state[7];
volatile bool is_key_triggered[7];
volatile bool key_was_target[7];
volatile uint8_t key_target_round[7];
volatile bool is_LED_active;
uint8_t     key_stable_cnt[7];
uint8_t     difficulty,level;
volatile uint8_t target1,target2,game_prog;
uint16_t    buffer1[512],buffer2[512];
uint16_t    play_prog1;
volatile uint16_t score;
volatile uint16_t play_prog2;
static volatile uint32_t gRandomSeed;
static uint8_t gGameProgressDrawn;
static volatile bool gRoundActive;
static volatile bool gScoreDirty;
static volatile uint8_t gTargetRound;
static volatile uint8_t gTargetMask;
static volatile uint8_t gHitMask;
static uint8_t gWelcomeLedStep;
static uint8_t gWelcomeLastTick;
static uint8_t gConfirmLastTick;

static SPIF_HandleTypeDef gSpifHandle;
SPIF_HandleTypeDef *Handle = &gSpifHandle;
static uint32_t gAudioChunkCount = 0;
static bool gSpifReadFail = false;
static volatile bool gAudioUnderflow = false;
static volatile uint16_t gAudioRing[AUDIO_RING_SAMPLES];
static volatile uint32_t gAudioRingRead = 0;
static volatile uint32_t gAudioRingWrite = 0;

void FillBuffer(void);

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
    buffer_state = IDLE_BUF2;
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
}

static void ClearAllLEDs(void)
{
    for (uint8_t i = 0; i < 7; i++)
    {
        DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
    }
    target1 = 8;
    target2 = 8;
    is_LED_active = false;
}

static void ShowOnlyLED(uint8_t led)
{
    for (uint8_t i = 0; i < 7; i++)
    {
        if (i == led)
        {
            DL_GPIO_setPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
        }
        else
        {
            DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
        }
    }
}

static void ClearKeyEvents(void)
{
    for (uint8_t i = 0; i < 7; i++)
    {
        is_key_triggered[i] = false;
        key_was_target[i] = false;
    }
}

static uint8_t NextTarget(void)
{
    gRandomSeed = gRandomSeed * 1664525U + 1013904223U;
    return (uint8_t)((gRandomSeed >> 16) % 7U);
}

static uint16_t GetBgmAddr(void)
{
    return (uint16_t)(BGM_ADDR0 + (uint16_t)level * BGM_LEN);
}

static uint8_t CountSetBits(uint8_t value)
{
    uint8_t count = 0;
    while (value != 0U)
    {
        count += value & 1U;
        value >>= 1U;
    }
    return count;
}

static void SpawnTargets(void)
{
    if (game_state == PLAYING && gRoundActive)
    {
        uint8_t remaining_targets =
            CountSetBits(gTargetMask & (uint8_t)~gHitMask);
        if (remaining_targets > 0U)
        {
            key_result = NO_HIT;
            play_prog2 = 1;
            if (score >= remaining_targets)
            {
                score -= remaining_targets;
            }
            else
            {
                score = 0;
            }
            gScoreDirty = true;
        }
    }

    ClearAllLEDs();
    gTargetRound++;
    gRoundActive = true;
    gHitMask = 0;
    target1 = NextTarget();
    gTargetMask = (uint8_t)(1U << target1);
    DL_GPIO_setPins((GPIO_Regs *)LEDs_PORT[target1], LEDs_PIN[target1]);
    if (difficulty >= 4)
    {
        do
        {
            target2 = NextTarget();
        } while (target2 == target1);
        gTargetMask |= (uint8_t)(1U << target2);
        DL_GPIO_setPins((GPIO_Regs *)LEDs_PORT[target2], LEDs_PIN[target2]);
    }
    is_LED_active = true;
}

static void ShowDifficultyScreen(void)
{
    ClearAllLEDs();
    OLED_Clear();
    OLED_ShowChinese(1, 2, 0);
    OLED_ShowChinese(1, 4, 1);
    for (uint8_t i = 0; i < 7; i++)
    {
        OLED_ShowChinese(2, i + 1, i + 4);
    }
}

static void ProcessGameKeys(void)
{
    for (uint8_t i = 0; i < 7; i++)
    {
        if (!is_key_triggered[i])
        {
            continue;
        }

        is_key_triggered[i] = false;
        uint8_t event_round = key_target_round[i];
        bool belongs_to_target = key_was_target[i];
        key_was_target[i] = false;

        if (belongs_to_target)
        {
            key_result = HIT;
            score += 3;
            if (event_round == gTargetRound && gRoundActive)
            {
                uint8_t key_bit = (uint8_t)(1U << i);
                gHitMask |= key_bit;
                DL_GPIO_clearPins((GPIO_Regs *)LEDs_PORT[i], LEDs_PIN[i]);
                if (i == target1)
                {
                    target1 = 8;
                }
                if (i == target2)
                {
                    target2 = 8;
                }
                if ((gHitMask & gTargetMask) == gTargetMask)
                {
                    gRoundActive = false;
                    is_LED_active = false;
                }
            }
        }
        else
        {
            key_result = MISS;
            if (score > 0)
            {
                score--;
            }
        }
        play_prog2 = 1;
        gScoreDirty = true;
    }
}

static void ResetProgressBar(void)
{
    OLED_SetCursor(PROGRESS_PAGE, PROGRESS_X);
    OLED_WriteData(0xFF);
    for (uint8_t i = 0; i < PROGRESS_INNER_WIDTH; i++)
    {
        OLED_WriteData(0x81);
    }
    OLED_WriteData(0xFF);
    gGameProgressDrawn = 0;
}

static void UpdateProgressBar(void)
{
    uint8_t progress = (uint8_t)(((uint32_t)game_prog *
        PROGRESS_INNER_WIDTH) / GAME_DURATION_TICKS);

    if (progress > PROGRESS_INNER_WIDTH)
    {
        progress = PROGRESS_INNER_WIDTH;
    }
    if (progress <= gGameProgressDrawn)
    {
        return;
    }

    OLED_SetCursor(PROGRESS_PAGE,
        PROGRESS_X + 1U + gGameProgressDrawn);
    while (gGameProgressDrawn < progress)
    {
        OLED_WriteData(0xFF);
        gGameProgressDrawn++;
    }
}

static void ShowResultBanner(char *text, bool visible)
{
    OLED_ShowString(2, 1, visible ? text : "      ");
}

static void PlayResultAudio(
    uint16_t address, uint16_t length, char *banner)
{
    uint8_t last_flash_phase = 0xFFU;

    play_prog1 = address;
    StartDMA();
    while (play_prog1 < address + length)
    {
        uint8_t flash_phase =
            (uint8_t)((play_prog1 - address) / RESULT_FLASH_CHUNKS);
        if (flash_phase != last_flash_phase)
        {
            ShowResultBanner(banner, (flash_phase & 1U) == 0U);
            last_flash_phase = flash_phase;
        }
        FillBuffer();
    }
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    ShowResultBanner(banner, true);
}

static void UpdateWelcomeLEDs(void)
{
    uint8_t tick = game_prog;
    if (tick == gWelcomeLastTick)
    {
        return;
    }
    gWelcomeLastTick = tick;
    ShowOnlyLED(gWelcomeLedStep);
    gWelcomeLedStep = (uint8_t)((gWelcomeLedStep + 1U) % 7U);
}

static void UpdateConfirmLED(void)
{
    uint8_t tick = game_prog;
    if (tick == gConfirmLastTick)
    {
        return;
    }
    gConfirmLastTick = tick;
    if ((tick & 1U) == 0U)
    {
        ShowOnlyLED(difficulty);
    }
    else
    {
        ClearAllLEDs();
    }
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



static void MixAlert(uint16_t *buffer)
{
    const uint16_t *samples;
    uint16_t sample_count;
    uint16_t total_samples;

    if (play_prog2 == 0)
    {
        return;
    }

    if (key_result == HIT)
    {
        samples = alert_hit;
        sample_count = ALERT_LEN;
        total_samples = ALERT_LEN;
    }
    else if (key_result == MISS)
    {
        samples = alert_miss;
        sample_count = ALERT_LEN;
        total_samples = ALERT_LEN * 3U;
    }
    else
    {
        samples = alert_timeout;
        sample_count = ALERT_TIMEOUT_LEN;
        total_samples = ALERT_TIMEOUT_LEN * 8U;
    }

    for (uint16_t i = 0; i < 512 && play_prog2 <= total_samples; i++)
    {
        buffer[i] = samples[(play_prog2 - 1U) % sample_count];
        play_prog2++;
    }
    if (play_prog2 > total_samples)
    {
        play_prog2 = 0;
    }
}

void FillBuffer(){
    if (buffer_state == FILL_BUF1){
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer2[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        LoadNextAudioChunk(buffer1);
        MixAlert(buffer1);
        buffer_state = IDLE_BUF1;
    }
    else if (buffer_state == FILL_BUF2){
        DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&buffer1[0]);
        DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)&(DAC0 -> DATA0));
        DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, 512);
        DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
        LoadNextAudioChunk(buffer2);
        MixAlert(buffer2);
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

#if !GAME_DEMO
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
#endif

#if GAME_DEMO
int main(void)
{
    SYSCFG_DL_init();
    OLED_Init();

    __NVIC_ClearPendingIRQ(DAC12_INT_IRQN);
    __NVIC_EnableIRQ(DAC12_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_KEYs_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_KEYs_INST_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_LEDs_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_LEDs_INST_INT_IRQN);
    __NVIC_ClearPendingIRQ(TIMER_PROG_INST_INT_IRQN);
    __NVIC_EnableIRQ(TIMER_PROG_INST_INT_IRQN);

    if (!SPIF_Init(Handle))
    {
        OLED_ShowString(1, 1, "SPIF INIT FAIL");
        while (1) {
        }
    }

    for (uint8_t i = 0; i < 7; i++)
    {
        key_state[i] = DL_GPIO_readPins((GPIO_Regs *)KEYs_PORT[i], KEYs_PIN[i]);
        is_key_triggered[i] = false;
        key_was_target[i] = false;
        key_target_round[i] = 0;
        key_stable_cnt[i] = 0;
    }

    game_state = WELCOME;
    buffer_state = IDLE_BUF2;
    diff_sel_state = IDLE;
    key_result = HIT;
    difficulty = 0;
    level = 0;
    game_prog = 0;
    play_prog1 = WELCOME_ADDR;
    play_prog2 = 0;
    gRandomSeed = 1;
    gTargetRound = 0;
    gTargetMask = 0;
    gHitMask = 0;
    gRoundActive = false;
    gScoreDirty = false;
    gWelcomeLedStep = 0;
    gWelcomeLastTick = 0xFF;
    ClearAllLEDs();
    OLED_ShowCoverIMG();
    DL_Timer_startCounter(TIMER_PROG_INST);
    StartDMA();

    while (1)
    {
        switch (game_state)
        {
            case WELCOME:
                while (play_prog1 < WELCOME_ADDR + WELCOME_LEN &&
                    game_prog < WELCOME_DURATION_TICKS)
                {
                    FillBuffer();
                    UpdateWelcomeLEDs();
                }
                DL_Timer_stopCounter(TIMER_PROG_INST);
                game_prog = 0;
                DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
                ShowDifficultyScreen();
                ClearKeyEvents();
                DL_Timer_startCounter(TIMER_KEYs_INST);
                game_state = DIFF_SEL;
                break;

            case DIFF_SEL:
                if (diff_sel_state == BUSY)
                {
                    while (play_prog1 < DIFF_ADDR0 + (difficulty + 1) * DIFF_LEN)
                    {
                        FillBuffer();
                    }
                    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
                    ClearKeyEvents();
                    DL_Timer_startCounter(TIMER_KEYs_INST);
                    diff_sel_state = SELECTED;
                }
                else if (diff_sel_state == CONFIRMED)
                {
                    while (play_prog1 < START_ADDR + START_LEN)
                    {
                        FillBuffer();
                        UpdateConfirmLED();
                    }
                    DL_Timer_stopCounter(TIMER_PROG_INST);
                    game_prog = 0;
                    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
                    DL_Timer_setLoadValue(TIMER_LEDs_INST, SPEED[difficulty]);
                    ClearAllLEDs();
                    level = 0;
                    score = 0;
                    diff_sel_state = IDLE;
                    game_state = LEVEL_SEL;
                }
                else
                {
                    for (uint8_t i = 0; i < 7; i++)
                    {
                        if (!is_key_triggered[i])
                        {
                            continue;
                        }
                        ClearKeyEvents();
                        DL_Timer_stopCounter(TIMER_KEYs_INST);
                        if (diff_sel_state == SELECTED && i == difficulty)
                        {
                            play_prog1 = START_ADDR;
                            diff_sel_state = CONFIRMED;
                            game_prog = 0;
                            gConfirmLastTick = 0xFF;
                            DL_Timer_startCounter(TIMER_PROG_INST);
                        }
                        else
                        {
                            play_prog1 = DIFF_ADDR0 + i * DIFF_LEN;
                            difficulty = i;
                            ShowOnlyLED(i);
                            OLED_ShowNum(1, 11, i + 1, 1);
                            diff_sel_state = BUSY;
                        }
                        StartDMA();
                        break;
                    }
                }
                break;

            case LEVEL_SEL:
                play_prog1 = LEVEL_ADDR0 + level * LEVEL_LEN;
                OLED_Clear();
                OLED_ShowChinese(1, 1, 2);
                OLED_ShowChinese(1, 3, 3);
                OLED_ShowNum(1, 4, level + 1, 1);
                OLED_ShowChinese(1, 5, 11);
                OLED_ShowChinese(1, 6, 12);
                OLED_ShowChar(1, 13, ':');
                OLED_ShowNum(1, 14, TARGET_SCORE[difficulty][level], 3);
                OLED_ShowChinese(2, 5, 13);
                OLED_ShowChinese(2, 6, 14);
                OLED_ShowChar(2, 13, ':');
                OLED_ShowNum(2, 14, score, 3);
                game_state = LEVEL_SEL_BUSY;
                StartDMA();
                break;

            case LEVEL_SEL_BUSY:
                while (play_prog1 < LEVEL_ADDR0 + (level + 1) * LEVEL_LEN)
                {
                    FillBuffer();
                }
                DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
                ClearKeyEvents();
                ClearAllLEDs();
                game_prog = 0;
                ResetProgressBar();
                play_prog1 = GetBgmAddr();
                SpawnTargets();
                DL_Timer_startCounter(TIMER_KEYs_INST);
                DL_Timer_startCounter(TIMER_LEDs_INST);
                DL_Timer_startCounter(TIMER_PROG_INST);
                game_state = PLAYING;
                StartDMA();
                break;

            case PLAYING:
                if (play_prog1 >= GetBgmAddr() + BGM_LEN)
                {
                    play_prog1 = GetBgmAddr();
                }
                ProcessGameKeys();
                FillBuffer();
                if (gScoreDirty)
                {
                    OLED_ShowNum(2, 14, score, 3);
                    gScoreDirty = false;
                }
                UpdateProgressBar();
                if (game_prog >= GAME_DURATION_TICKS)
                {
                    DL_Timer_stopCounter(TIMER_KEYs_INST);
                    DL_Timer_stopCounter(TIMER_LEDs_INST);
                    DL_Timer_stopCounter(TIMER_PROG_INST);
                    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
                    ClearKeyEvents();
                    ClearAllLEDs();
                    game_prog = 0;
                    game_state = GAME_OVER;
                }
                break;

            case GAME_OVER:
                if (score >= TARGET_SCORE[difficulty][level])
                {
                    if (level < 2)
                    {
                        PlayResultAudio(WIN_ADDR, WIN_LEN, "WIN!!!");
                        level++;
                        game_state = LEVEL_SEL;
                    }
                    else
                    {
                        PlayResultAudio(
                            WIN_ALL_ADDR, WIN_ALL_LEN, "WIN!!!");

                        if (difficulty == 6U)
                        {
                            OLED_Clear();
                            OLED_ShowString(1, 5, "Password:");
                            OLED_ShowString(2, 7, "3618");
                            play_prog1 = HIDDEN_WIN_ADDR;
                            StartDMA();
                            while (play_prog1 <
                                HIDDEN_WIN_ADDR + HIDDEN_WIN_LEN)
                            {
                                FillBuffer();
                            }
                            DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
                            ClearKeyEvents();
                            game_state = GAME_FINISHED;
                            break;
                        }

                        diff_sel_state = IDLE;
                        ShowDifficultyScreen();
                        ClearKeyEvents();
                        game_state = DIFF_SEL;
                        DL_Timer_startCounter(TIMER_KEYs_INST);
                    }
                }
                else
                {
                    PlayResultAudio(LOSE_ADDR, LOSE_LEN, "LOSE...");
                    diff_sel_state = IDLE;
                    ShowDifficultyScreen();
                    ClearKeyEvents();
                    game_state = DIFF_SEL;
                    DL_Timer_startCounter(TIMER_KEYs_INST);
                }
                break;

            case GAME_FINISHED:
                break;

            default:
                game_state = WELCOME;
                break;
        }
    }
}
#endif

void DAC12_IRQHandler(void)
{
    if (DL_DAC12_getPendingInterrupt(DAC0) != DL_DAC12_IIDX_DMA_DONE)
    {
        return;
    }

    if (buffer_state == IDLE_BUF1)
    {
        buffer_state = FILL_BUF2;
    }
    else if (buffer_state == IDLE_BUF2)
    {
        buffer_state = FILL_BUF1;
    }
}

void TIMER_KEYs_INST_IRQHandler(){
    switch (DL_Timer_getPendingInterrupt(TIMER_KEYs_INST)){
        case DL_TIMER_IIDX_ZERO:
        {
            bool current_state;
            gRandomSeed++;
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
                            uint8_t key_bit = (uint8_t)(1U << i);
                            key_target_round[i] = gTargetRound;
                            key_was_target[i] =
                                (game_state == PLAYING) &&
                                gRoundActive &&
                                ((gTargetMask & key_bit) != 0U) &&
                                ((gHitMask & key_bit) == 0U);
                            is_key_triggered[i] = true;
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
            SpawnTargets();
        break;
        default:break;
    }
}

void TIMER_PROG_INST_IRQHandler(){
    switch (DL_Timer_getPendingInterrupt(TIMER_PROG_INST)){
        case DL_TIMER_IIDX_ZERO:
            if (game_prog < GAME_DURATION_TICKS)
            {
                game_prog++;
            }
        break;
        default:break;
    }
}
