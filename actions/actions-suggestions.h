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

#ifndef LIBTEXTCLASSIFIER_ACTIONS_ACTIONS_SUGGESTIONS_H_
#define LIBTEXTCLASSIFIER_ACTIONS_ACTIONS_SUGGESTIONS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "actions/actions_model_generated.h"
#include "actions/feature-processor.h"
#include "actions/ngram-model.h"
#include "actions/ranker.h"
#include "actions/types.h"
#include "annotator/annotator.h"
#include "annotator/model-executor.h"
#include "annotator/types.h"
#include "utils/flatbuffers.h"
#include "utils/i18n/locale.h"
#include "utils/memory/mmap.h"
#include "utils/tflite-model-executor.h"
#include "utils/utf8/unilib.h"
#include "utils/variant.h"
#include "utils/zlib/zlib.h"

namespace libtextclassifier3 {

// Options for suggesting actions.
struct ActionSuggestionOptions {
  static ActionSuggestionOptions Default() { return ActionSuggestionOptions(); }
};

// Class for predicting actions following a conversation.
class ActionsSuggestions {
 public:
  static std::unique_ptr<ActionsSuggestions> FromUnownedBuffer(
      const uint8_t* buffer, const int size, const UniLib* unilib = nullptr,
      const std::string& triggering_preconditions_overlay = "");
  // Takes ownership of the mmap.
  static std::unique_ptr<ActionsSuggestions> FromScopedMmap(
      std::unique_ptr<libtextclassifier3::ScopedMmap> mmap,
      const UniLib* unilib = nullptr,
      const std::string& triggering_preconditions_overlay = "");
  static std::unique_ptr<ActionsSuggestions> FromFileDescriptor(
      const int fd, const int offset, const int size,
      const UniLib* unilib = nullptr,
      const std::string& triggering_preconditions_overlay = "");
  static std::unique_ptr<ActionsSuggestions> FromFileDescriptor(
      const int fd, const UniLib* unilib = nullptr,
      const std::string& triggering_preconditions_overlay = "");
  static std::unique_ptr<ActionsSuggestions> FromPath(
      const std::string& path, const UniLib* unilib = nullptr,
      const std::string& triggering_preconditions_overlay = "");

  ActionsSuggestionsResponse SuggestActions(
      const Conversation& conversation,
      const ActionSuggestionOptions& options = ActionSuggestionOptions()) const;

  ActionsSuggestionsResponse SuggestActions(
      const Conversation& conversation, const Annotator* annotator,
      const ActionSuggestionOptions& options = ActionSuggestionOptions()) const;

  const ActionsModel* model() const;
  const reflection::Schema* entity_data_schema() const;

  // Should be in sync with those defined in Android.
  // android/frameworks/base/core/java/android/view/textclassifier/ConversationActions.java
  static const std::string& kViewCalendarType;
  static const std::string& kViewMapType;
  static const std::string& kTrackFlightType;
  static const std::string& kOpenUrlType;
  static const std::string& kSendSmsType;
  static const std::string& kCallPhoneType;
  static const std::string& kSendEmailType;
  static const std::string& kShareLocation;

 private:
  struct CompiledRule {
    const RulesModel_::Rule* rule;
    std::unique_ptr<UniLib::RegexPattern> pattern;
    std::unique_ptr<UniLib::RegexPattern> output_pattern;
    CompiledRule(const RulesModel_::Rule* rule,
                 std::unique_ptr<UniLib::RegexPattern> pattern,
                 std::unique_ptr<UniLib::RegexPattern> output_pattern)
        : rule(rule),
          pattern(std::move(pattern)),
          output_pattern(std::move(output_pattern)) {}
  };

  // Checks that model contains all required fields, and initializes internal
  // datastructures.
  bool ValidateAndInitialize();

  void SetOrCreateUnilib(const UniLib* unilib);

  // Initializes regular expression rules.
  bool InitializeRules(ZlibDecompressor* decompressor);
  bool InitializeRules(ZlibDecompressor* decompressor, const RulesModel* rules,
                       std::vector<CompiledRule>* compiled_rules) const;

  // Prepare preconditions.
  // Takes values from flag provided data, but falls back to model provided
  // values for parameters that are not explicitly provided.
  bool InitializeTriggeringPreconditions();

  // Extracts input token features.
  bool ExtractTokenFeatures(const std::vector<std::string>& context,
                            std::vector<float>* embeddings,
                            std::vector<int>* num_tokens_per_message,
                            int* max_num_tokens_per_message) const;

  bool AllocateInput(const int conversation_length, const int max_tokens,
                     tflite::Interpreter* interpreter) const;

  bool SetupModelInput(const std::vector<std::string>& context,
                       const std::vector<int>& user_ids,
                       const std::vector<float>& time_diffs,
                       const int num_suggestions,
                       const float confidence_threshold,
                       const float diversification_distance,
                       const float empirical_probability_factor,
                       tflite::Interpreter* interpreter) const;
  bool ReadModelOutput(tflite::Interpreter* interpreter,
                       const ActionSuggestionOptions& options,
                       ActionsSuggestionsResponse* response) const;

  bool SuggestActionsFromModel(
      const Conversation& conversation, const int num_messages,
      const ActionSuggestionOptions& options,
      ActionsSuggestionsResponse* response,
      std::unique_ptr<tflite::Interpreter>* interpreter) const;

  // Creates options for annotation of a message.
  AnnotationOptions AnnotationOptionsForMessage(
      const ConversationMessage& message) const;

  void SuggestActionsFromAnnotations(
      const Conversation& conversation, const ActionSuggestionOptions& options,
      const Annotator* annotator, std::vector<ActionSuggestion>* actions) const;

  void SuggestActionsFromAnnotation(
      const int message_index, const ActionSuggestionAnnotation& annotation,
      std::vector<ActionSuggestion>* actions) const;

  // Deduplicates equivalent annotations - annotations that have the same type
  // and same span text.
  // Returns the indices of the deduplicated annotations.
  std::vector<int> DeduplicateAnnotations(
      const std::vector<ActionSuggestionAnnotation>& annotations) const;

  bool SuggestActionsFromRules(const Conversation& conversation,
                               std::vector<ActionSuggestion>* actions) const;

  bool SuggestActionsFromLua(
      const Conversation& conversation,
      const TfLiteModelExecutor* model_executor,
      const tflite::Interpreter* interpreter,
      const reflection::Schema* annotation_entity_data_schema,
      std::vector<ActionSuggestion>* actions) const;

  bool GatherActionsSuggestions(const Conversation& conversation,
                                const Annotator* annotator,
                                const ActionSuggestionOptions& options,
                                ActionsSuggestionsResponse* response) const;

  // Checks whether the input triggers the low confidence checks.
  bool IsLowConfidenceInput(const Conversation& conversation,
                            const int num_messages,
                            std::vector<int>* post_check_rules) const;
  // Checks and filters suggestions triggering the low confidence post checks.
  bool FilterConfidenceOutput(const std::vector<int>& post_check_rules,
                              std::vector<ActionSuggestion>* actions) const;

  // Returns whether a regex rule provides entity data from a match.
  bool HasEntityData(const RulesModel_::Rule* rule) const;

  ActionSuggestion SuggestionFromSpec(
      const ActionSuggestionSpec* action, const std::string& default_type = "",
      const std::string& default_response_text = "",
      const std::string& default_serialized_entity_data = "",
      const float default_score = 0.0f,
      const float default_priority_score = 0.0f) const;

  bool FillAnnotationFromMatchGroup(
      const UniLib::RegexMatcher* matcher,
      const RulesModel_::Rule_::RuleActionSpec_::CapturingGroup* group,
      const int message_index, ActionSuggestionAnnotation* annotation) const;

  const ActionsModel* model_;
  std::unique_ptr<libtextclassifier3::ScopedMmap> mmap_;

  // Tensorflow Lite models.
  std::unique_ptr<const TfLiteModelExecutor> model_executor_;

  // Rules.
  std::vector<CompiledRule> rules_, low_confidence_rules_;

  std::unique_ptr<UniLib> owned_unilib_;
  const UniLib* unilib_;

  // Locales supported by the model.
  std::vector<Locale> locales_;

  // Builder for creating extra data.
  const reflection::Schema* entity_data_schema_;
  std::unique_ptr<ReflectiveFlatbufferBuilder> entity_data_builder_;
  std::unique_ptr<ActionsSuggestionsRanker> ranker_;

  std::string lua_bytecode_;

  // Triggering preconditions. These parameters can be backed by the model and
  // (partially) be provided by flags.
  TriggeringPreconditionsT preconditions_;
  std::string triggering_preconditions_overlay_buffer_;
  const TriggeringPreconditions* triggering_preconditions_overlay_;

  // Feature extractor and options.
  std::unique_ptr<const ActionsFeatureProcessor> feature_processor_;
  std::unique_ptr<const EmbeddingExecutor> embedding_executor_;
  std::vector<float> embedded_padding_token_;
  int token_embedding_size_;

  // Low confidence input ngram classifier.
  std::unique_ptr<const NGramModel> ngram_model_;
};

// Interprets the buffer as a Model flatbuffer and returns it for reading.
const ActionsModel* ViewActionsModel(const void* buffer, int size);

// Opens model from given path and runs a function, passing the loaded Model
// flatbuffer as an argument.
//
// This is mainly useful if we don't want to pay the cost for the model
// initialization because we'll be only reading some flatbuffer values from the
// file.
template <typename ReturnType, typename Func>
ReturnType VisitActionsModel(const std::string& path, Func function) {
  ScopedMmap mmap(path);
  if (!mmap.handle().ok()) {
    function(/*model=*/nullptr);
  }
  const ActionsModel* model =
      ViewActionsModel(mmap.handle().start(), mmap.handle().num_bytes());
  return function(model);
}

}  // namespace libtextclassifier3

#endif  // LIBTEXTCLASSIFIER_ACTIONS_ACTIONS_SUGGESTIONS_H_