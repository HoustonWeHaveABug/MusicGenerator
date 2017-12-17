#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NOTES_N_MAX 4
#define FRACTION_SIZE 2
#define CHORD5_SIZE 3
#define CHORD7_SIZE 4
#define SCALE_SIZE 7
#define CHORD_POSITION_FREQS_SIZE (SCALE_SIZE*2)
#define TONIC_NOTE_MAX 11
#define NOTE_DURATION_MIN 5
#define PITCH_MAX 127
#define OCTAVE_SIZE 12

typedef enum {
	CHORD5,
	CHORD7
}
chord_class_t;

typedef struct {
	int chord_position;
	chord_class_t chord_class;
	int notes_n;
	int notes[NOTES_N_MAX];
	int start;
	int duration;
}
set_t;

typedef struct {
	int depth_max;
	int split_frac[FRACTION_SIZE];
	int merge_frac[FRACTION_SIZE];
	int richness5_freqs[CHORD5_SIZE+1];
	int richness7_freqs[CHORD7_SIZE+1];
	int pitch_min;
	int pitch_max;
	int ref_change_frac[FRACTION_SIZE];
	int pitch_ref;
	int duration;
	int sets_n;
	set_t *sets;
}
line_t;

typedef struct {
	int tonic_note;
	int scale[SCALE_SIZE];
	int bars_n;
	int bar_size;
	int bar_duration;
	int chord_position_freqs[CHORD_POSITION_FREQS_SIZE];
	line_t chords;
	line_t melody;
	int duration;
}
piece_t;

int read_piece_settings(piece_t *);
int read_line_settings(line_t *, const char *, int, int);
int read_frequencies(int [], int, const char *, const char *);
int create_piece(piece_t *);
int create_chords_bar(piece_t *, int, int);
int create_chords_sets(piece_t *, int, int);
int create_melody_bar(piece_t *, int, int, int, chord_class_t);
int create_melody_sets(piece_t *, int, int, int, chord_class_t);
int add_set_to_line(piece_t *, line_t *, int, int, chord_class_t);
set_t *create_set(line_t *);
int choose_notes(piece_t *, line_t *, int [], int [], int, int, int []);
int compare_ints(const void *, const void *);
void copy_notes(int [], int, int []);
void notes_best_octave(line_t *, int [], int, int *, int *);
void shift_notes(int [], int, int, int []);
void increment_set_duration(set_t *, int);
int test_fraction(int []);
int erand(int, int);
void print_piece(piece_t *, int);
void print_line(line_t *, int);
void print_set(set_t *, int);
void free_pieces(int);
void free_line(line_t *);

int scale_major[SCALE_SIZE] = { 0, 2, 4, 5, 7, 9, 11 }, scale_minor[SCALE_SIZE] = { 0, 2, 3, 5, 7, 8, 11 }, chord5_idx[CHORD5_SIZE] = { 0, 2, 4 }, chord7_idx[CHORD7_SIZE] = { 0, 2, 4, 6 }, half_frac[FRACTION_SIZE] = { 1, 2 };
piece_t *pieces;

int main(void) {
	int pieces_n, pieces_idx, start_offset;
	if (scanf("%d", &pieces_n) != 1 || pieces_n < 1) {
		fprintf(stderr, "Invalid number of pieces\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	pieces = malloc(sizeof(piece_t)*(size_t)pieces_n);
	if (!pieces) {
		fprintf(stderr, "Could not allocate memory for pieces\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	srand((unsigned)time(NULL));
	for (pieces_idx = 0; pieces_idx < pieces_n; pieces_idx++) {
		if (!read_piece_settings(pieces+pieces_idx)) {
			free_pieces(pieces_idx);
			return EXIT_FAILURE;
		}
		if (!create_piece(pieces+pieces_idx)) {
			free_pieces(pieces_idx+1);
			return EXIT_FAILURE;
		}
	}
	start_offset = 0;
	while (scanf("%d", &pieces_idx) == 1) {
		if (pieces_idx < 0 || pieces_idx >= pieces_n) {
			fprintf(stderr, "Invalid piece index\n");
			fflush(stderr);
		}
		print_piece(pieces+pieces_idx, start_offset);
		start_offset += pieces[pieces_idx].duration;
	}
	free_pieces(pieces_n);
	return EXIT_SUCCESS;
}

int read_piece_settings(piece_t *piece) {
	int minor_flag, scale_idx;
	if (scanf("%d", &piece->tonic_note) != 1 || piece->tonic_note < 0 || piece->tonic_note > TONIC_NOTE_MAX) {
		fprintf(stderr, "Invalid piece tonic note\n");
		fflush(stderr);
		return 0;
	}
	if (scanf("%d", &minor_flag) != 1) {
		fprintf(stderr, "Invalid piece minor flag\n");
		fflush(stderr);
		return 0;
	}
	if (minor_flag) {
		for (scale_idx = 0; scale_idx < SCALE_SIZE; scale_idx++) {
			piece->scale[scale_idx] = scale_minor[scale_idx];
		}
	}
	else {
		for (scale_idx = 0; scale_idx < SCALE_SIZE; scale_idx++) {
			piece->scale[scale_idx] = scale_major[scale_idx];
		}
	}
	if (scanf("%d", &piece->bars_n) != 1 || piece->bars_n < 1) {
		fprintf(stderr, "Invalid piece number of bars\n");
		fflush(stderr);
		return 0;
	}
	if (scanf("%d", &piece->bar_size) != 1 || piece->bar_size < 1) {
		fprintf(stderr, "Invalid piece bar size\n");
		fflush(stderr);
		return 0;
	}
	if (scanf("%d", &piece->bar_duration) != 1 || piece->bar_duration < 1) {
		fprintf(stderr, "Invalid piece bar duration\n");
		fflush(stderr);
		return 0;
	}
	if (!read_frequencies(piece->chord_position_freqs, CHORD_POSITION_FREQS_SIZE, "piece", "chord position frequencies")) {
		return 0;
	}
	if (!read_line_settings(&piece->chords, "chords", piece->bar_size, piece->bar_duration)) {
		return 0;
	}
	if (!read_line_settings(&piece->melody, "melody", piece->bar_size, piece->bar_duration)) {
		return 0;
	}
	if (piece->melody.pitch_min <= piece->chords.pitch_max) {
		fprintf(stderr, "Conflict between chords maximum pitch and melody minimum pitch\n");
		fflush(stderr);
		return 0;
	}
	piece->duration = piece->bars_n*piece->bar_duration;
	return 1;
}

int read_line_settings(line_t *line, const char *name, int bar_size, int bar_duration) {
	int depth;
	if (scanf("%d", &line->depth_max) != 1 || line->depth_max < 0) {
		fprintf(stderr, "Invalid %s maximum depth\n", name);
		fflush(stderr);
		return 0;
	}
	for (depth = 0; depth < line->depth_max && bar_duration%bar_size == 0 && bar_duration/bar_size >= NOTE_DURATION_MIN; depth++) {
		bar_duration /= bar_size;
	}
	if (depth < line->depth_max) {
		fprintf(stderr, "Depth of %s is not compatible with bar\n", name);
		fflush(stderr);
		return 0;
	}
	if (!read_frequencies(line->split_frac, FRACTION_SIZE, name, "split fraction")) {
		return 0;
	}
	if (!read_frequencies(line->merge_frac, FRACTION_SIZE, name, "merge fraction")) {
		return 0;
	}
	if (!read_frequencies(line->richness5_freqs, CHORD5_SIZE+1, name, "richness5")) {
		return 0;
	}
	if (!read_frequencies(line->richness7_freqs, CHORD7_SIZE+1, name, "richness7")) {
		return 0;
	}
	if (scanf("%d", &line->pitch_min) != 1 || line->pitch_min < 0 || line->pitch_min > PITCH_MAX) {
		fprintf(stderr, "Invalid %s minimum pitch\n", name);
		return 0;
	}
	if (scanf("%d", &line->pitch_max) != 1 || line->pitch_max < 0 || line->pitch_max > PITCH_MAX || line->pitch_max < line->pitch_min || line->pitch_max-line->pitch_min+1 > RAND_MAX) {
		fprintf(stderr, "Invalid %s maximum pitch\n", name);
		return 0;
	}
	if (!read_frequencies(line->ref_change_frac, FRACTION_SIZE, name, "reference change fraction")) {
		return 0;
	}
	line->pitch_ref = erand(line->pitch_max-line->pitch_min+1, line->pitch_min);
	line->duration = 0;
	line->sets_n = 0;
	line->sets = NULL;
	return 1;
}

int read_frequencies(int frequencies[], int frequencies_size, const char *entity, const char *name) {
	int frequencies_idx;
	if (scanf("%d", frequencies) != 1 || frequencies[0] < 0) {
		fprintf(stderr, "Invalid %s %s (index 0)\n", entity, name);
		return 0;
	}
	for (frequencies_idx = 1; frequencies_idx < frequencies_size; frequencies_idx++) {
		if (scanf("%d", frequencies+frequencies_idx) != 1 || frequencies[frequencies_idx] < 0) {
			fprintf(stderr, "Invalid %s %s (index %d)\n", entity, name, frequencies_idx);
			return 0;
		}
		frequencies[frequencies_idx] += frequencies[frequencies_idx-1];
	}
	if (frequencies[frequencies_size-1] == 0 || frequencies[frequencies_size-1] > RAND_MAX) {
		fprintf(stderr, "Invalid %s %s (sum = 0 or greater than RAND_MAX)\n", entity, name);
		return 0;
	}
	return 1;
}

int create_piece(piece_t *piece) {
	int bars_idx;
	for (bars_idx = 0; bars_idx < piece->bars_n; bars_idx++) {
		if (!create_chords_bar(piece, 0, piece->bar_duration)) {
			return 0;
		}
	}
	return 1;
}

int create_chords_bar(piece_t *piece, int depth, int bar_duration) {
	int split = create_chords_sets(piece, depth, bar_duration/piece->bar_size), bar_idx;
	if (split < 0) {
		return 0;
	}
	for (bar_idx = 1; bar_idx < piece->bar_size; bar_idx++) {
		if (!split && test_fraction(piece->chords.merge_frac)) {
			increment_set_duration(piece->chords.sets+piece->chords.sets_n-1, bar_duration/piece->bar_size);
			piece->chords.duration += bar_duration/piece->bar_size;
			if (create_melody_sets(piece, depth, bar_duration/piece->bar_size, piece->chords.sets[piece->chords.sets_n-1].chord_position, piece->chords.sets[piece->chords.sets_n-1].chord_class) < 0) {
				return 0;
			}
		}
		else {
			split = create_chords_sets(piece, depth, bar_duration/piece->bar_size);
			if (split < 0) {
				return 0;
			}
		}
	}
	return 1;
}

int create_chords_sets(piece_t *piece, int depth, int beat_duration) {
	int split = depth < piece->chords.depth_max && test_fraction(piece->chords.split_frac);
	if (split) {
		if (!create_chords_bar(piece, depth+1, beat_duration)) {
			return -1;
		}
	}
	else {
		int chord_position_freqs_rand = erand(piece->chord_position_freqs[CHORD_POSITION_FREQS_SIZE-1], 0), chord_position, chord_class;
		for (chord_position = 0; chord_position < SCALE_SIZE && piece->chord_position_freqs[chord_position] <= chord_position_freqs_rand; chord_position++);
		if (chord_position < SCALE_SIZE) {
			chord_class = CHORD5;
		}
		else {
			for (chord_position = 0; chord_position < SCALE_SIZE && piece->chord_position_freqs[SCALE_SIZE+chord_position] <= chord_position_freqs_rand; chord_position++);
			chord_class = CHORD7;
		}
		if (!add_set_to_line(piece, &piece->chords, beat_duration, chord_position, chord_class)) {
			return -1;
		}
		if (create_melody_sets(piece, depth, beat_duration, chord_position, chord_class) < 0) {
			return -1;
		}
	}
	return split;
}

int create_melody_bar(piece_t *piece, int depth, int bar_duration, int chord_position, chord_class_t chord_class) {
	int split = create_melody_sets(piece, depth, bar_duration/piece->bar_size, chord_position, chord_class), bar_idx;
	if (split < 0) {
		return 0;
	}
	for (bar_idx = 1; bar_idx < piece->bar_size; bar_idx++) {
		if (!split && test_fraction(piece->melody.merge_frac)) {
			increment_set_duration(piece->melody.sets+piece->melody.sets_n-1, bar_duration/piece->bar_size);
			piece->melody.duration += bar_duration/piece->bar_size;
		}
		else {
			split = create_melody_sets(piece, depth, bar_duration/piece->bar_size, chord_position, chord_class);
			if (split < 0) {
				return 0;
			}
		}
	}
	return 1;
}

int create_melody_sets(piece_t *piece, int depth, int beat_duration, int chord_position, chord_class_t chord_class) {
	int split = depth < piece->melody.depth_max && test_fraction(piece->melody.split_frac);
	if (split) {
		if (!create_melody_bar(piece, depth+1, beat_duration, chord_position, chord_class)) {
			return -1;
		}
	}
	else {
		if (!add_set_to_line(piece, &piece->melody, beat_duration, chord_position, chord_class)) {
			return -1;
		}
	}
	return split;
}

int add_set_to_line(piece_t *piece, line_t *line, int beat_duration, int chord_position, chord_class_t chord_class) {
	set_t *set = create_set(line);
	if (!set) {
		return 0;
	}
	set->chord_position = chord_position;
	set->chord_class = chord_class;
	if (chord_class == CHORD5) {
		set->notes_n = choose_notes(piece, line, line->richness5_freqs, chord5_idx, CHORD5_SIZE, chord_position, set->notes);
	}
	else if (chord_class == CHORD7) {
		set->notes_n = choose_notes(piece, line, line->richness7_freqs, chord7_idx, CHORD7_SIZE, chord_position, set->notes);
	}
	set->start = line->duration;
	set->duration = beat_duration;
	if (test_fraction(half_frac)) {
		while (line->pitch_ref > line->pitch_min && test_fraction(line->ref_change_frac)) {
			line->pitch_ref--;
		}
	}
	else {
		while (line->pitch_ref < line->pitch_max && test_fraction(line->ref_change_frac)) {
			line->pitch_ref++;
		}
	}
	line->duration += beat_duration;
	return 1;
}

set_t *create_set(line_t *line) {
	if (line->sets_n == 0) {
		line->sets = malloc(sizeof(set_t));
		if (!line->sets) {
			fprintf(stderr, "Could not allocate memory for line->sets\n");
			return NULL;
		}
	}
	else {
		set_t *sets_tmp = realloc(line->sets, sizeof(set_t)*(size_t)(line->sets_n+1));
		if (!sets_tmp) {
			fprintf(stderr, "Could not reallocate memory for line->sets\n");
			return NULL;
		}
		line->sets = sets_tmp;
	}
	line->sets_n++;
	return line->sets+line->sets_n-1;
}

int choose_notes(piece_t *piece, line_t *line, int richness_freqs[], int chord_idx[], int chord_size, int chord_position, int notes[]) {
	int richness_freqs_rand = erand(richness_freqs[chord_size], 0), notes_n, notes_tmp[NOTES_N_MAX], notes_idx;

	/* Choose notes according to chord position and richness */
	for (notes_n = 0; notes_n <= chord_size && richness_freqs[notes_n] <= richness_freqs_rand; notes_n++);
	for (notes_idx = 0; notes_idx < notes_n; notes_idx++) {
		int prev_idx;
		do {
			notes_tmp[notes_idx] = piece->scale[(chord_idx[erand(chord_size, 0)]+chord_position)%SCALE_SIZE]+piece->tonic_note;
			for (prev_idx = 0; prev_idx < notes_idx && notes_tmp[prev_idx] != notes_tmp[notes_idx]; prev_idx++);
		}
		while (prev_idx < notes_idx);
	}
	if (notes_n > 0) {
		int shift_best = 0, variance_best, octave_best, shift, notes_min, notes_max;

		/* Sort selected notes */
		qsort(notes_tmp, (size_t)notes_n, sizeof(int), compare_ints);

		/* Choose shift and octave that best fit pitch reference */
		copy_notes(notes_tmp, notes_n, notes);
		notes_best_octave(line, notes, notes_n, &variance_best, &octave_best);
		for (shift = 1; shift < notes_n; shift++) {
			int variance, octave;
			shift_notes(notes_tmp, notes_n, shift, notes);
			notes_best_octave(line, notes, notes_n, &variance, &octave);
			if (variance < variance_best) {
				shift_best = shift;
				variance_best = variance;
				octave_best = octave;
			}
		}
		if (shift_best == 0) {
			copy_notes(notes_tmp, notes_n, notes);
		}
		else {
			shift_notes(notes_tmp, notes_n, shift_best, notes);
		}
		for (notes_idx = 0; notes_idx < notes_n; notes_idx++) {
			notes[notes_idx] += OCTAVE_SIZE*octave_best;
		}

		/* Remove notes not included in pitch range */
		for (notes_min = 0; notes_min < notes_n && notes[notes_min] < line->pitch_min; notes_min++);
		for (notes_max = notes_n; notes_max > 0 && notes[notes_max-1] > line->pitch_max; notes_max--);
		for (notes_idx = notes_min; notes_idx < notes_max; notes_idx++) {
			notes[notes_idx-notes_min] = notes[notes_idx];
		}
		notes_n = notes_max-notes_min;
	}
	return notes_n;
}

int compare_ints(const void *a, const void *b) {
	const int *int_a = (const int *)a, *int_b = (const int *)b;
	return *int_a-*int_b;
}

void copy_notes(int notes_a[], int notes_n, int notes_b[]) {
	int notes_idx;
	for (notes_idx = 0; notes_idx < notes_n; notes_idx++) {
		notes_b[notes_idx] = notes_a[notes_idx];
	}
}

void notes_best_octave(line_t *line, int notes[], int notes_n, int *variance, int *octave) {
	int notes_idx;
	*variance = 0;
	for (notes_idx = 0; notes_idx < notes_n; notes_idx++) {
		*variance += abs(notes[notes_idx]-line->pitch_ref);
	}
	*octave = 0;
	do {
		int variance_next = 0;
		for (notes_idx = 0; notes_idx < notes_n; notes_idx++) {
			variance_next += abs(notes[notes_idx]+OCTAVE_SIZE-line->pitch_ref);
		}
		if (variance_next < *variance) {
			for (notes_idx = 0; notes_idx < notes_n; notes_idx++) {
				notes[notes_idx] += OCTAVE_SIZE;
			}
			*variance = variance_next;
			*octave = *octave+1;
		}
		else {
			break;
		}
	}
	while (1);
}

void shift_notes(int notes_a[], int notes_n, int shift, int notes_b[]) {
	int notes_idx;
	for (notes_idx = 0; notes_idx < notes_n-shift; notes_idx++) {
		notes_b[notes_idx+shift] = notes_a[notes_idx]+OCTAVE_SIZE;
	}
	for (; notes_idx < notes_n; notes_idx++) {
		notes_b[notes_idx+shift-notes_n] = notes_a[notes_idx];
	}
}

void increment_set_duration(set_t *set, int duration) {
	set->duration += duration;
}

int test_fraction(int frac[]) {
	return erand(frac[1], 0) < frac[0];
}

int erand(int range, int offset) {
	return (int)(rand()/(RAND_MAX+1.0)*range)+offset;
}

void print_piece(piece_t *piece, int start_offset) {
	print_line(&piece->chords, start_offset);
	print_line(&piece->melody, start_offset);
}

void print_line(line_t *line, int start_offset) {
	int sets_idx;
	for (sets_idx = 0; sets_idx < line->sets_n; sets_idx++) {
		print_set(line->sets+sets_idx, start_offset);
	}
}

void print_set(set_t *set, int start_offset) {
	int notes_idx;
	for (notes_idx = 0; notes_idx < set->notes_n; notes_idx++) {
		printf("%d %.2lf %.2f\n", set->notes[notes_idx], (set->start+start_offset)/100.0, set->duration/100.0);
	}
}

void free_pieces(int pieces_max) {
	int pieces_idx;
	for (pieces_idx = 0; pieces_idx < pieces_max; pieces_idx++) {
		free_line(&pieces[pieces_idx].melody);
		free_line(&pieces[pieces_idx].chords);
	}
	free(pieces);
}

void free_line(line_t *line) {
	if (line->sets) {
		free(line->sets);
	}
}
