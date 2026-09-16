#include "sequencer.h"
#include "synth.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  SAMPLE_RATE = 44100,
  BPM = 120,
  TOTAL_MEASURES = 4,
};

static void write_wav(const char *path, const int16_t *samples, size_t count) {
  FILE *file = fopen(path, "wb");
  if (file == NULL) {
    perror(path);
    exit(1);
  }
  uint32_t riff_size = (uint32_t)(36 + count * sizeof(*samples));
  uint32_t fmt_size = 16;
  uint16_t format = 1;
  uint16_t channels = 1;
  uint32_t sample_rate = SAMPLE_RATE;
  uint32_t byte_rate = SAMPLE_RATE * sizeof(*samples);
  uint16_t block_align = sizeof(*samples);
  uint16_t bits_per_sample = 16;
  fwrite("RIFF", 4, 1, file);
  fwrite(&riff_size, sizeof(riff_size), 1, file);
  fwrite("WAVEfmt ", 8, 1, file);
  fwrite(&fmt_size, sizeof(fmt_size), 1, file);
  fwrite(&format, sizeof(format), 1, file);
  fwrite(&channels, sizeof(channels), 1, file);
  fwrite(&sample_rate, sizeof(sample_rate), 1, file);
  fwrite(&byte_rate, sizeof(byte_rate), 1, file);
  fwrite(&block_align, sizeof(block_align), 1, file);
  fwrite(&bits_per_sample, sizeof(bits_per_sample), 1, file);
  fwrite("data", 4, 1, file);
  uint32_t data_size = (uint32_t)(count * sizeof(*samples));
  fwrite(&data_size, sizeof(data_size), 1, file);
  fwrite(samples, sizeof(*samples), count, file);
  fclose(file);
}

static void add_step(Pattern *pattern, uint8_t pitch, uint8_t step, uint8_t velocity) {
  uint8_t measure = step / 4;
  uint8_t subdivision = step % 4;
  measure_add_note(&pattern->measures[measure], pitch, velocity, 24,
                   subdivision * 24);
}

int main(void) {
  Sequence sequence = {0};
  sequence.pattern_count = 1;
  sequence.bpm = BPM;
  sequence.patterns[0].measure_count = 4;
  for (uint8_t measure = 0; measure < 4; measure++) {
    sequence.patterns[0].measures[measure].beats_per_measure = 4;
    sequence.patterns[0].measures[measure].beat_unit = 4;
  }
  Pattern *pattern = &sequence.patterns[0];
  for (uint8_t step = 0; step < 16; step += 4) {
    add_step(pattern, 36, step, 120);
    add_step(pattern, 42, step, 80);
  }
  for (uint8_t step = 2; step < 16; step += 4) add_step(pattern, 42, step, 65);
  for (uint8_t step = 4; step < 16; step += 8) add_step(pattern, 38, step, 110);
  add_step(pattern, 46, 14, 100);

  size_t sample_count = (size_t)(60.0 * 4 * TOTAL_MEASURES / BPM * SAMPLE_RATE) +
                        SAMPLE_RATE / 2;
  int16_t *samples = calloc(sample_count, sizeof(*samples));
  if (samples == NULL) return 1;

  Synth synth;
  synth_init(&synth, SAMPLE_RATE);
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, synth_handle_event, &synth);
  if (!sequencer_play(&sequencer, 0)) return 1;

  size_t rendered = 0;
  uint32_t total_ticks = PPQN * 4 * TOTAL_MEASURES;
  for (uint32_t tick = 1; tick <= total_ticks; tick++) {
    sequencer_update(&sequencer, tick);
    size_t target = (size_t)((double)tick * SAMPLE_RATE * 60.0 /
                             (BPM * PPQN));
    synth_render(&synth, samples + rendered, target - rendered);
    rendered = target;
  }
  sequencer_stop(&sequencer, total_ticks);
  synth_render(&synth, samples + rendered, sample_count - rendered);

  write_wav("/tmp/drum-machine-demo.wav", samples, sample_count);
  free(samples);
  printf("Wrote /tmp/drum-machine-demo.wav\n");
  return 0;
}
