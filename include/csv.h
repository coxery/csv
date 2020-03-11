/*
 _______  _______  __   __
|      _||       ||  | |  |  Fast CSV Parser for Modern C++
|     |  |  _____||  |_|  |  http://github.com/p-ranav/csv
|     |  | |_____ |       |
|     |  |_____  ||       |
|     |_  _____| | |     |
|_______||_______|  |___|

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2019 Pranav Srinivas Kumar <pranav.srinivas.kumar@gmail.com>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <unordered_map>
#include <fstream>
#include <vector>
#include <string>

using string_map = std::unordered_map<std::string_view, std::string>;

namespace csv {

  struct Dialect {

    std::string delimiter_;
    bool skip_initial_space_;
    char line_terminator_;
    char quote_character_;
    bool double_quote_;
    std::vector<char> trim_characters_;
    bool header_;
    bool skip_empty_rows_;
      
    std::unordered_map<std::string_view, bool> ignore_columns_;
    std::vector<std::string> column_names_;

    Dialect() :
      delimiter_(","),
      skip_initial_space_(false),
      line_terminator_('\n'),
      quote_character_('"'),
      double_quote_(true),
      trim_characters_({}),
      header_(true),
      skip_empty_rows_(false) {}

    Dialect& delimiter(const std::string& delimiter) {
      delimiter_ = delimiter;
      return *this;
    }

    Dialect& skip_initial_space(bool skip_initial_space) {
      skip_initial_space_ = skip_initial_space;
      return *this;
    }

    Dialect& skip_empty_rows(bool skip_empty_rows) {
      skip_empty_rows_ = skip_empty_rows;
      return *this;
    }

    Dialect& quote_character(char quote_character) {
      quote_character_ = quote_character;
      return *this;
    }

    Dialect& double_quote(bool double_quote) {
      double_quote_ = double_quote;
      return *this;
    }

    // Base case for trim_characters parameter packing
    Dialect& trim_characters() {
      return *this;
    }

    // Parameter packed trim_characters method
    // Accepts a variadic number of characters
    template<typename T, typename... Targs>
    Dialect& trim_characters(T character, Targs... Fargs) {
      trim_characters_.push_back(character);
      trim_characters(Fargs...);
      return *this;
    }

    // Base case for ignore_columns parameter packing
    Dialect& ignore_columns() {
      return *this;
    }

    // Parameter packed trim_characters method
    // Accepts a variadic number of columns
    template<typename T, typename... Targs>
    Dialect& ignore_columns(T column, Targs... Fargs) {
      ignore_columns_[column] = true;
      ignore_columns(Fargs...);
      return *this;
    }

    // Base case for ignore_columns parameter packing
    Dialect& column_names() {
      return *this;
    }
    
    // Some utility structs to check template specialization
    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type {};
    
    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref> : std::true_type {};        

    // Parameter packed trim_characters method
    // Accepts a variadic number of columns
    template<typename T, typename... Targs>
    typename
    std::enable_if<!is_specialization<T, std::vector>::value, Dialect&>::type
    column_names(T column, Targs... Fargs) {
      column_names_.push_back(column);
      column_names(Fargs...);
      return *this;
    }
    
    // Parameter packed trim_characters method
    // Accepts a vector of strings
    Dialect& column_names(const std::vector<std::string>& columns) {
      for (auto& column : columns)
	column_names_.push_back(column);
      return *this;
    }

    Dialect& header(bool header) {
      header_ = header;
      return *this;
    }
  };

  class Reader {
  public:
    Reader() :
      columns_(0),
      current_dialect_name_("excel"),
      expected_number_of_rows_(0),
      ignore_columns_enabled_(false),
      trimming_enabled_(false) {

      Dialect unix_dialect;
      unix_dialect
        .delimiter(",")
        .quote_character('"')
        .double_quote(true)
        .header(true);
      dialects_["unix"] = unix_dialect;

      Dialect excel_dialect;
      excel_dialect
        .delimiter(",")
        .quote_character('"')
        .double_quote(true)
        .header(true);
      dialects_["excel"] = excel_dialect;

      Dialect excel_tab_dialect;
      excel_tab_dialect
        .delimiter("\t")
        .quote_character('"')
        .double_quote(true)
        .header(true);
      dialects_["excel_tab"] = excel_tab_dialect;
    }

    ~Reader() {
    }

    void read(const std::string& filename, size_t rows = 0) {
      current_dialect_ = dialects_[current_dialect_name_];

      std::ifstream stream(filename);
      if (!stream.is_open()) {
        throw std::runtime_error("error: Failed to open " + filename);
      }

      if (rows > 0)
      {
		  expected_number_of_rows_ = rows;
      }
      else
      {
          // new lines will be skipped unless we stop it from happening:    
          stream.unsetf(std::ios_base::skipws);
          std::string line;
          while (std::getline(stream, line)) {
              if (line.size() > 0 && line[line.size() - 1] == '\r')
                  line.pop_back();
              if (line != "" || (!current_dialect_.skip_empty_rows_ && line == ""))
                  ++expected_number_of_rows_;
          }

          if (current_dialect_.header_ && expected_number_of_rows_ > 0)
              expected_number_of_rows_ -= 1;

          stream.clear();
          stream.seekg(0, std::ios::beg);
      }

      if (current_dialect_.trim_characters_.size() > 0)
        trimming_enabled_ = true;

      if (current_dialect_.ignore_columns_.size() > 0)
        ignore_columns_enabled_ = true;

      read_internal(stream);
    }

    Dialect& configure_dialect(const std::string& dialect_name = "excel") {
      if (dialects_.find(dialect_name) != dialects_.end()) {
        return dialects_[dialect_name];
      }
      else {
        dialects_[dialect_name] = Dialect();
        current_dialect_name_ = dialect_name;
        return dialects_[dialect_name];
      }
    }

    std::vector<std::string> list_dialects() {
      std::vector<std::string> result;
      for (auto&kvpair : dialects_)
        result.push_back(kvpair.first);
      return result;
    }

    Dialect& get_dialect(const std::string& dialect_name) {
      return dialects_[dialect_name];
    }

    void use_dialect(const std::string& dialect_name) {
      current_dialect_name_ = dialect_name;
      if (dialects_.find(dialect_name) == dialects_.end()) {
        throw std::runtime_error("error: Dialect " + dialect_name + " not found");
      }
    }

    std::vector<std::unordered_map<std::string_view, std::string>> rows() {
      return rows_;
    }

    std::vector<std::string> cols() {
      return headers_;
    }

    std::pair<size_t, size_t> shape() {
      return { expected_number_of_rows_, columns_ };
    }

  private:
    void read_internal(std::ifstream& stream) {
      // Get current position
      std::streamoff length = stream.tellg();

      // Get first line and find headers by splitting on delimiters
      std::string first_line;
      getline(stream, first_line);

      // Under Linux, getline removes \n from the input stream. 
      // However, it does not remove the \r
      // Let's remove it
      if (first_line.size() > 0 && first_line[first_line.size() - 1] == '\r') {
        first_line.pop_back();
      }

      auto split_result = split(first_line);
      if (current_dialect_.header_) {
        headers_ = split_result;
      }
      else {
        headers_.clear();
        if (current_dialect_.column_names_.size() > 0) {
          headers_ = current_dialect_.column_names_;
        }
        else {
          for (size_t i = 0; i < split_result.size(); i++)
            headers_.push_back(std::to_string(i));
        }
        // return to start before getline()
        stream.seekg(length, std::ios_base::beg);
      }

      columns_ = headers_.size();

      string_map current_row;
      for (auto& header : headers_)
        current_row[header] = "";
      if (ignore_columns_enabled_)
        for (auto&kvpair : current_dialect_.ignore_columns_)
          current_row.erase(kvpair.first);

      // Get lines one at a time, split on the delimiter
      bool skip_empty_rows = current_dialect_.skip_empty_rows_;
      auto ignore_columns = current_dialect_.ignore_columns_;
      std::string row;
      std::string_view column_name;
      size_t number_of_rows = 0;

      while (std::getline(stream, row)) {
        if (number_of_rows == expected_number_of_rows_)
          break;
        if (row.size() > 0 && row[row.size() - 1] == '\r')
          row.pop_back();
        if (row != "" || (!skip_empty_rows && row == "")) {
          split_result = split(row);
          size_t i = 0;
          for (auto& value : split_result)
          {
            column_name = headers_[i++];
            if (!ignore_columns_enabled_ || ignore_columns.count(column_name) == 0)
              current_row[column_name] = value;
          }

          rows_.push_back(current_row);
          number_of_rows += 1;
        }
      }

      stream.close();
    }

    // trim white spaces from the left end of an input string
    std::string ltrim(std::string const& input) {
      std::string result = input;
      result.erase(result.begin(), std::find_if(result.begin(), result.end(), [=](int ch) {
        return !(std::find(current_dialect_.trim_characters_.begin(), current_dialect_.trim_characters_.end(), ch)
          != current_dialect_.trim_characters_.end());
      }));
      return result;
    }

    // trim white spaces from right end of an input string
    std::string rtrim(std::string const& input) {
      std::string result = input;
      result.erase(std::find_if(result.rbegin(), result.rend(), [=](int ch) {
        return !(std::find(current_dialect_.trim_characters_.begin(), current_dialect_.trim_characters_.end(), ch)
          != current_dialect_.trim_characters_.end());
      }).base(), result.end());
      return result;
    }

    // trim white spaces from either end of an input string
    std::string trim(std::string const& input) {
      if (current_dialect_.trim_characters_.size() == 0)
        return input;
      return ltrim(rtrim(input));
    }

    // split string based on a delimiter sub-string
    std::vector<std::string> split(std::string const& input_string) {
      std::vector<std::string> result;

      if (input_string == "") {
        result = std::vector<std::string>(columns_, "");
      }

      std::string sub_result = "";
      bool discard_delimiter = false;
      size_t quotes_encountered = 0;
      size_t input_string_size = input_string.size();

      for (size_t i = 0; i < input_string_size; ++i) {

        // Check if ch is the start of a delimiter sequence
        bool delimiter_detected = false;
        for (size_t j = 0; j < current_dialect_.delimiter_.size(); ++j) {

          char ch = input_string[i];
          if (ch != current_dialect_.delimiter_[j]) {
            delimiter_detected = false;
            break;
          }
          else {
            // ch *might* be the start of a delimiter sequence
            if (j + 1 == current_dialect_.delimiter_.size()) {
              if (quotes_encountered % 2 == 0) {
                // Reached end of delimiter sequence without breaking
                // delimiter detected!
                delimiter_detected = true;
                result.push_back(trimming_enabled_ ? trim(sub_result) : sub_result);
                sub_result = "";

                // If enabled, skip initial space right after delimiter
                if (i + 1 < input_string_size) {
                  if (current_dialect_.skip_initial_space_ && input_string[i + 1] == ' ') {
                    i = i + 1;
                  }
                }
                quotes_encountered = 0;
              }
              else {
                sub_result += input_string[i];
                i = i + 1;
                if (i == input_string_size) break;
              }
            }
            else {
              // Keep looking
              i = i + 1;
              if (i == input_string_size) break;
            }
          }
        }

        // base case
        if (!delimiter_detected)
          sub_result += input_string[i];

        if (input_string[i] == current_dialect_.quote_character_)
          quotes_encountered += 1;
        if (input_string[i] == current_dialect_.quote_character_ &&
          current_dialect_.double_quote_ &&
          sub_result.size() >= 2 &&
          sub_result[sub_result.size() - 2] == input_string[i])
          quotes_encountered -= 1;
      }

      if (sub_result != "")
        result.push_back(trimming_enabled_ ? trim(sub_result) : sub_result);

      if (result.size() < columns_) {
        for (size_t i = result.size(); i < columns_; i++) {
          result.push_back("");
        }
      }
      else if (result.size() > columns_ && columns_ != 0) {
        result.resize(columns_);
      }
      return result;
    }

    std::vector<std::string> headers_;
    std::vector<std::unordered_map<std::string_view, std::string>> rows_;

    // Member variables to keep track of rows/cols
    size_t columns_;
    size_t expected_number_of_rows_;

    std::string current_dialect_name_;
    std::unordered_map<std::string, Dialect> dialects_;
    Dialect current_dialect_;
    bool ignore_columns_enabled_;
    bool trimming_enabled_;
  };

  // Some utility structs to check template specialization
  template<typename Test, template<typename...> class Ref>
  struct is_specialization : std::false_type {};

  template<template<typename...> class Ref, typename... Args>
  struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

  class Writer {
  public:
      explicit Writer(const std::string& file_name) :
          header_written_(false),
          current_dialect_name_("excel") {
          stream_.open(file_name);
          Dialect unix_dialect;
          unix_dialect
              .delimiter(",")
              .quote_character('"')
              .double_quote(true)
              .header(true);
          dialects_["unix"] = unix_dialect;

          Dialect excel_dialect;
          excel_dialect
              .delimiter(",")
              .quote_character('"')
              .double_quote(true)
              .header(true);
          dialects_["excel"] = excel_dialect;

          Dialect excel_tab_dialect;
          excel_tab_dialect
              .delimiter("\t")
              .quote_character('"')
              .double_quote(true)
              .header(true);
          dialects_["excel_tab"] = excel_tab_dialect;
      }

      ~Writer() {
          close();
      }

      Dialect& configure_dialect(const std::string& dialect_name = "excel") {
          if (dialects_.find(dialect_name) != dialects_.end()) {
              return dialects_[dialect_name];
          }
          else {
              dialects_[dialect_name] = Dialect();
              current_dialect_name_ = dialect_name;
              return dialects_[dialect_name];
          }
      }

      std::vector<std::string> list_dialects() {
          std::vector<std::string> result;
          for (auto& kvpair : dialects_)
              result.push_back(kvpair.first);
          return result;
      }

      Dialect& get_dialect(const std::string& dialect_name) {
          return dialects_[dialect_name];
      }

      void use_dialect(const std::string& dialect_name) {
          current_dialect_name_ = dialect_name;
          if (dialects_.find(dialect_name) == dialects_.end()) {
              throw std::runtime_error("error: Dialect " + dialect_name + " not found");
          }
      }

      template<typename T, typename... Targs>
      typename std::enable_if<is_specialization<T, std::map>::value
          || is_specialization<T, std::unordered_map>::value, void>::type
          write_row(T row_map) {
          const auto& column_names = dialects_[current_dialect_name_].column_names_;
          for (size_t i = 0; i < column_names.size(); i++) {
              current_row_entries_.push_back(row_map[column_names[i]]);
          }
          write_row();
      }

      void write_row(const std::vector<std::string>& row_entries) {
          current_row_entries_ = row_entries;
          write_row();
      }

      void write_row() {
          if (!header_written_) {
              write_header();
              header_written_ = true;
          }
          std::string row;
          for (size_t i = 0; i < current_row_entries_.size(); i++) {
              row += current_row_entries_[i];
              if (i + 1 < current_row_entries_.size())
                  row += dialects_[current_dialect_name_].delimiter_;
          }
          row += dialects_[current_dialect_name_].line_terminator_;
          stream_ << row;
          current_row_entries_.clear();
      }

      // Parameter packed write_row method
      // Accepts a variadic number of entries
      template<typename T, typename... Targs>
      typename std::enable_if<!is_specialization<T, std::vector>::value, void>::type
          write_row(T entry, Targs... Fargs) {
          current_row_entries_.push_back(entry);
          write_row(Fargs...);
      }

      void close() {
          stream_.close();
      }

  private:

      void write_header() {
          auto dialect = dialects_[current_dialect_name_];
          auto column_names = dialect.column_names_;
          if (column_names.size() == 0)
              return;
          auto delimiter = dialect.delimiter_;
          auto line_terminator = dialect.line_terminator_;
          std::string row;
          for (size_t i = 0; i < column_names.size(); i++) {
              row += column_names[i];
              if (i + 1 < column_names.size())
                  row += delimiter;
          }
          row += line_terminator;
          stream_ << row;
      }

      Dialect current_dialect_;
      bool header_written_;

      std::ofstream stream_;
      std::vector<std::string> current_row_entries_;

      std::string current_dialect_name_;
      std::unordered_map<std::string, Dialect> dialects_;
  };

}
