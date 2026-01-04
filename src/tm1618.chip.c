/* TM1618 Custom chip implementation 
Made with Gemini AI, using oh so many prompts...
Afterwards checked the code and fixed small things.
*/

// Wokwi Custom Chip - For docs and examples see:
// https://docs.wokwi.com/chips-api/getting-started
//

////////////////////////////////

#include "wokwi-api.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  pin_t pin_stb;
  pin_t pin_clk;
  pin_t pin_dio;
  pin_t pin_k1;
  pin_t seg_pins[8];   
  pin_t grid_pins[4];  
  
  uint8_t ram[14];     
  uint8_t current_byte;
  int bit_idx;
  uint8_t address;
  bool auto_inc;
  bool display_on;
  int active_grid;
  bool command_mode;
  bool reading_keys; 
  uint8_t key_data[3]; // TM1618 returns 3 bytes for keys
  bool is_scanning; // true = Scan Phase, false = Display Phase
} chip_state_t;

static void scan_keys(chip_state_t *chip) {
  // 1. Blank display during scan to prevent ghosting
  for (int i = 0; i < 4; i++) pin_write(chip->grid_pins[i], HIGH);
  
  // 2. Clear key registers for fresh scan
  memset(chip->key_data, 0, 3);

  // 3. Pulse KS1-KS5 LOW and sample K1
  for (int ks = 0; ks < 5; ks++) {
    // Set KS lines HIGH, then pull target LOW
    for(int j = 0; j < 5; j++) pin_write(chip->seg_pins[j], HIGH);
    pin_write(chip->seg_pins[ks], LOW);
    
    // Sample K1 pin
    if (pin_read(chip->pin_k1) == LOW) {
      switch(ks) {
        case 0: chip->key_data[0] |= (1 << 1); break; // KS1
        case 1: chip->key_data[0] |= (1 << 4); break; // KS2
        case 2: chip->key_data[1] |= (1 << 1); break; // KS3
        case 3: chip->key_data[1] |= (1 << 4); break; // KS4
        case 4: chip->key_data[2] |= (1 << 1); break; // KS5
      }
    }
  }
}

static void refresh_display(chip_state_t *chip) {
  if (!chip->display_on) return;

  // 1. Prepare segment data using 5+3 mapping
  uint8_t even_val = chip->ram[chip->active_grid * 2];
  uint8_t odd_val  = chip->ram[(chip->active_grid * 2) + 1];
  
  // SEG1-5 from Even addr [4:0], SEG6-8 from Odd addr [5:3]
  uint8_t full_seg = (even_val & 0x1F) | (((odd_val >> 3) & 0x07) << 5);

  // 2. Drive segments
  for (int i = 0; i < 8; i++) {
    pin_write(chip->seg_pins[i], (full_seg & (1 << i)) ? HIGH : LOW);
  }

  // 3. Activate current grid
  pin_write(chip->grid_pins[chip->active_grid], LOW);
  
  // 4. Move to next grid for next refresh cycle
  chip->active_grid = (chip->active_grid + 1) % 4;
}

static void on_timer_tick(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  if (chip->is_scanning) {
    scan_keys(chip);
    chip->is_scanning = false; // Next tick will display
  } else {
    refresh_display(chip);
    chip->is_scanning = true; // Next tick will display
  }
}

/*
static void on_timer_tick(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  if (chip->is_scanning) {
    // --- PHASE 1: KEY SCANNING ---
    // Blank display (Common Cathode: Grids HIGH = OFF)
    for (int i = 0; i < 4; i++) pin_write(chip->grid_pins[i], HIGH);
    
    // Reset key registers
    memset(chip->key_data, 0, 3);

    for (int ks = 0; ks < 5; ks++) {
      // Set KS lines HIGH, then pulse the target LOW
      for(int j=0; j<5; j++) pin_write(chip->seg_pins[j], HIGH);
      pin_write(chip->seg_pins[ks], LOW);
      
      // Check K1 status
      if (pin_read(chip->pin_k1) == LOW) {
        if (ks == 0) chip->key_data[0] |= (1 << 1); // KS1
        if (ks == 1) chip->key_data[0] |= (1 << 4); // KS2
        if (ks == 2) chip->key_data[1] |= (1 << 1); // KS3
        if (ks == 3) chip->key_data[1] |= (1 << 4); // KS4
        if (ks == 4) chip->key_data[2] |= (1 << 1); // KS5
      }
    }
    chip->is_scanning = false; // Next tick will display
  } 
  else {
    // --- PHASE 2: DISPLAY REFRESH ---
    if (chip->display_on) {
      uint8_t even_val = chip->ram[chip->active_grid * 2];
      uint8_t odd_val  = chip->ram[(chip->active_grid * 2) + 1];
      
      // Mapping: SEG1-5 (Even [4:0]), SEG6-8 (Odd [5:3])
      uint8_t full_seg = (even_val & 0x1F) | (((odd_val >> 3) & 0x07) << 5);

      for (int i = 0; i < 8; i++) {
        pin_write(chip->seg_pins[i], (full_seg & (1 << i)) ? HIGH : LOW);
      }

      pin_write(chip->grid_pins[chip->active_grid], LOW);
      chip->active_grid = (chip->active_grid + 1) % 4;
    }
    chip->is_scanning = true; // Next tick will scan
  }
}
*/

void process_byte(chip_state_t *chip) {
  uint8_t b = chip->current_byte;
  if (!chip->command_mode) {
    // Write Data to RAM
    if (chip->address < 14) {
      //printf("data %02x = %02x.\n", chip->address, b);
      chip->ram[chip->address] = b;
      if (chip->auto_inc) {
        chip->address++;
        if (chip->address >= 14) chip->address = 0;
      }
    }
  }
  else  { // (chip->command_mode)
    if ((b & 0xc0) == 0x00) { 
      // Display Mode Setting
      printf("process_byte: %02x = Display Mode.\n", b);
    } else if ((b & 0xc0) == 0x40) { 
      // Data Setting
      //printf("process_byte: %02x = Data Setting.\n", b);
      if (b & 0x02) { // Read Key Command (0x42)
        chip->reading_keys = true;
        chip->bit_idx = 0;
        pin_mode(chip->pin_dio, OUTPUT);
      }
      chip->auto_inc = !(b & 0x04);  // auto increment addressing on/off 0x44/0x40
      chip->command_mode = false; // Next bytes are data/address
    } else if ((b & 0xc0) == 0xc0) { 
      // Address Setting
      chip->address = b & 0x0f;
      chip->command_mode = false; // Next bytes are data/address
      //printf("process_byte: %02x = Address %x data: %02x .\n", b, b & 0x0f, chip->ram[chip->address]);
    } else if ((b & 0xc0) == 0x80) { 
      // Display Control
      printf("process_byte: %02x = Display Control %02x.\n", b, b & 0x08);
      chip->display_on = (b & 0x08);
      printf("display: %s\n", chip->display_on?"on":"off");
      // todo: brightness
    }
  }
}

void on_pin_change(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;

  if (pin == chip->pin_stb) {
    if (value == HIGH) { 
      chip->bit_idx = 0; 
      chip->current_byte = 0;
      chip->command_mode = true; 
      chip->reading_keys = false;
      pin_mode(chip->pin_dio, INPUT);
    }
    return;
  }

  // Data is clocked on rising edge
  if (pin == chip->pin_clk) {
    if (value == HIGH && !chip->reading_keys) {
      if (pin_read(chip->pin_dio))
        chip->current_byte |= (1 << chip->bit_idx);
      chip->bit_idx++;

      if (chip->bit_idx == 8) {        
        process_byte(chip);
        chip->bit_idx = 0;
        chip->current_byte = 0;
      }
    } 
    else if (value == LOW && chip->reading_keys) {
      int byte_idx = chip->bit_idx / 8;
      int bit_in_byte = chip->bit_idx % 8;
      
      if (byte_idx < 3) {
        pin_write(chip->pin_dio, (chip->key_data[byte_idx] >> bit_in_byte) & 0x01 ? HIGH : LOW);
        chip->bit_idx++;
      }
    }
  }

}

void chip_init() {
  printf("chip_init start\n");
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip->ram, 0, 14);
  chip->display_on = false;
  chip->active_grid = 0;
  chip->command_mode = true;

  chip->pin_stb = pin_init("STB", INPUT);
  chip->pin_clk = pin_init("CLK", INPUT);
  chip->pin_dio = pin_init("DIO", INPUT);
  chip->pin_k1  = pin_init("K1", INPUT_PULLUP);

  // Segment Map based on your list
  const char *seg_names[] = {"SEG1", "SEG2", "SEG3", "SEG4", "SEG5", "SEG6", "SEG7", "SEG8"};
  for (int i = 0; i < 8; i++) chip->seg_pins[i] = pin_init(seg_names[i], OUTPUT);

  // Grid Map based on your list
  const char *grid_names[] = {"GRID1", "GRID2", "GRID3", "GRID4"};
  for (int i = 0; i < 4; i++) {
    chip->grid_pins[i] = pin_init(grid_names[i], OUTPUT);
    pin_write(chip->grid_pins[i], HIGH);
  }

  const pin_watch_config_t watch_config = { .edge = BOTH, .pin_change = on_pin_change, .user_data = chip };
  pin_watch(chip->pin_stb, &watch_config);
  pin_watch(chip->pin_clk, &watch_config);

  const timer_config_t timer_config = { .callback = on_timer_tick, .user_data = chip };
  timer_t timer = timer_init(&timer_config);
  timer_start(timer, 500, true); // 500 microsecond intervals

  printf("chip_init done\n");
}
