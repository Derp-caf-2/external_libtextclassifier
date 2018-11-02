/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_UTILS_SENTENCEPIECE_ENCODER_H_
#define LIBTEXTCLASSIFIER_UTILS_SENTENCEPIECE_ENCODER_H_

#include <vector>

#include "utils/base/logging.h"
#include "utils/sentencepiece/matcher.h"
#include "utils/strings/stringpiece.h"

namespace libtextclassifier3 {

// Encoder to segment/tokenize strings into pieces such that the sum of the
// scores of the pieces used is maximized.
class Encoder {
 public:
  // matcher: the list of valid sentence pieces represented as a matcher, e.g.
  //     a trie.
  // num_pieces: the number of pieces in the trie.
  // pieces_scores: the scores of the individual pieces.
  // start_code: Code that is used as encoding of the start of input.
  // end_code: Code that is used as encoding of the end of input.
  // encoding_offset: Value added to the sentence piece ids to make them
  //     not interesecting with start_code and end_code.
  Encoder(const SentencePieceMatcher* matcher, const int num_pieces,
          const float* pieces_scores, int start_code = 0, int end_code = 1,
          int encoding_offset = 2)
      : num_pieces_(num_pieces),
        scores_(pieces_scores),
        matcher_(matcher),
        start_code_(start_code),
        end_code_(end_code),
        encoding_offset_(encoding_offset) {}

  // Segment the input so that the total score of the pieces used is maximized.
  // This is a simplified implementation of the general Viterbi algorithm,
  // assuming independence between individual pieces.
  std::vector<int> Encode(StringPiece normalized_text) const;

 private:
  // State in the dynamic programming algorithm.
  struct SegmentationEntry {
    // Accumulated score.
    float score;

    // Position before last piece.
    int previous_pos;

    // Last piece used.
    int piece_id;

    // Total number of pieces used.
    int num_pieces;
  };

  const int num_pieces_;
  const float* scores_;
  const SentencePieceMatcher* matcher_;
  const int start_code_;
  const int end_code_;
  const int encoding_offset_;
};

}  // namespace libtextclassifier3

#endif  // LIBTEXTCLASSIFIER_UTILS_SENTENCEPIECE_ENCODER_H_
