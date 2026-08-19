#include "../include/parrallel.h"

static bool bit_get(const std::vector<uint8_t>& bits, uint32_t index) {
    return (bits[index / 8] >> (index % 8)) & 1;
}

static void bit_set(std::vector<uint8_t>& bits, uint32_t index) {
    bits[index / 8] |= static_cast<uint8_t>(1 << (index % 8));
}

bool is_piece_downloaded(GlobalTorrentState& global_state, uint32_t index) {
    std::lock_guard<std::mutex> lock(global_state.queue_mutex);
    if (index >= global_state.downloaded_pieces.size() * 8) return true;
    return bit_get(global_state.downloaded_pieces, index);
}

void mark_piece_downloaded(GlobalTorrentState& global_state, uint32_t index) {
    std::lock_guard<std::mutex> lock(global_state.queue_mutex);
    if (bit_get(global_state.downloaded_pieces, index)) return;
    bit_set(global_state.downloaded_pieces, index);
    global_state.pieces_finished++;
    if (global_state.pieces_finished >= global_state.total_pieces) {
        global_state.done = true;
    }
}

int get_work(GlobalTorrentState& global_state) {
    std::lock_guard<std::mutex> lock(global_state.queue_mutex);
    
    while (!global_state.missing_pieces.empty()) {
        uint32_t piece_to_do = global_state.missing_pieces.front();
        global_state.missing_pieces.pop();

        if (!bit_get(global_state.downloaded_pieces, piece_to_do)) {
            return piece_to_do;
        }
    }
    
    return -1; 
} 