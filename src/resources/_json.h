
enum Token {
    StartObject,
    EndObject,
    StartArray,
    EndArray,
    Unknown,
    End,
    String,
    Colon,
    Comma,
    Number,
};

struct TokenData {
    Token id;
    std::string_view string;
    std::string_view number;
};

const char* token_names[10] = {
    "start object",
    "end object",
    "start array",
    "end array",
    "unknown",
    "end",
    "string",
    "colon",
    "comma",
    "number"};

struct JsonTokenizer {
    size_t index;

    TokenData next(const std::vector<char>& contents) {
        if (index >= contents.size()) {
            return {.id = Token::End};
        }

        char c;

        bool is_known = true;

        bool in_string = false;
        uint32_t string_start_index;

        bool in_number = false;
        uint32_t number_start_index;

        while (is_known) {
            c = contents[index];
            index += 1;

            if (in_string) {
                if (c == '"') {
                    in_string = false;
                    return {
                        .id = Token::String,
                        .string = std::string_view(
                            &contents[string_start_index],
                            index - 1 - string_start_index
                        )};
                } else {
                    continue;
                }
            }

            if ((c >= '0' && c <= '9') || c == '0' || c == '.' || c == '-') {
                if (!in_number) {
                    number_start_index = index;
                }
                in_number = true;
                continue;
            } else if (in_number) {
                return {
                    .id = Token::Number,
                    .number = std::string_view(
                        &contents[number_start_index - 1],
                        index - (number_start_index)
                    )};
            }

            switch (c) {
                case '{':
                    return {.id = Token::StartObject};
                case '}':
                    return {.id = Token::EndObject};
                case '[':
                    return {.id = Token::StartArray};
                case ']':
                    return {.id = Token::EndArray};
                case ':':
                    return {.id = Token::Colon};
                case '"':
                    string_start_index = index;
                    in_string = true;
                    break;
                case ',':
                case ' ':
                case '\n':
                    break;
                default:
                    is_known = false;
                    break;
            }
        }

        dbg(c);

        return {.id = Token::Unknown};
    }
};

void load_gltf(const std::string& filepath) {
    std::ifstream file_stream(filepath);

    if (!file_stream) {
        dbg(filepath);
        abort();
    }

    std::vector<char> contents(
        (std::istreambuf_iterator<char>(file_stream)),
        {}
    );

    auto tokenizer = JsonTokenizer {.index = 0};

    while (true) {
        auto tok = tokenizer.next(contents);
        if (tok.id == Token::String) {
            dbg(tok.string);
        } else if (tok.id == Token::Number) {
            dbg(tok.number);
        } else {
            dbg(token_names[tok.id]);
        }
        if (tok.id == Token::End) {
            break;
        }
    }
}
