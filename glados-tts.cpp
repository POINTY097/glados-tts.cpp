#include <torch/script.h>
#include <iostream>
#include <vector>
#include <string>
#include <espeak-ng/speak_lib.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

using namespace std;

const vector<string> ones {"","one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
const vector<string> teens {"ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen","sixteen", "seventeen", "eighteen", "nineteen"};
const vector<string> tens {"", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
const vector<tuple<string, string>> abbreviations {{"°C", "degrees selsius"}, {"°F", "degrees fahrenheit"}, {"°c", "degrees selsius"}, {"°f", "degrees fahrenheit"}, {"°", "degrees"}, {"hPa", "hecto pascals"}, {"g/m³", "grams per cubic meter"}, {"% {RH}", "percent relative humidity"}, {"mrs", "misess"}, {"mr", "mister"}, {"dr", "doctor"}};
const vector<char> punctuations {';', ':', ',', '.', '!', '?', '"', '\'', '(', ')'};
const vector<string> all_phonemes = {"_", "!", "\'", "(", ")", ",", ".", ":", ";", "?", " ", "-", "i", "y", "ɨ", "ʉ", "ɯ", "u", "ɪ", "ʏ", "ʊ", "e", "ø", "ɘ", "ə", "ɵ", "ɤ", "o", "ɛ", "œ", "ɜ", "ɞ", "ʌ", "ɔ", "æ", "ɐ", "a", "ɶ", "ɑ", "ɒ", "ᵻ", "ʘ", "ɓ", "ǀ", "ɗ", "ǃ", "ʄ", "ǂ", "ɠ", "ǁ", "ʛ", "p", "b", "t", "d", "ʈ", "ɖ", "c", "ɟ", "k", "ɡ", "q", "ɢ", "ʔ", "ɴ", "ŋ", "ɲ", "ɳ", "n", "ɱ", "m", "ʙ", "r", "ʀ", "ⱱ", "ɾ", "ɽ", "ɸ", "β", "f", "v", "θ", "ð", "s", "z", "ʃ", "ʒ", "ʂ", "ʐ", "ç", "ʝ", "x", "ɣ", "χ", "ʁ", "ħ", "ʕ", "h", "ɦ", "ɬ", "ɮ", "ʋ", "ɹ", "ɻ", "j", "ɰ", "l", "ɭ", "ʎ", "ʟ", "ˈ", "ˌ", "ː", "ˑ", "ʍ", "w", "ɥ", "ʜ", "ʢ", "ʡ", "ɕ", "ʑ", "ɺ", "ɧ", "ɚ", "˞ɫ"};

class glados
{
private:
    static string nameForNumber (long number)
    {
        if (number == 0)
        {
            return "zero";
        }
        else if (number < 10)
        {
            return ones[number];
        }
        else if (number < 20)
        {
            return teens [number - 10];
        }
        else if (number < 100)
        {
            return tens[number / 10] + ((number % 10 != 0) ? " " + nameForNumber(number % 10) : "");
        }
        else if (number < 1000)
        {
            return nameForNumber(number / 100) + " hundred" + ((number % 100 != 0) ? " " + nameForNumber(number % 100) : "");
        }
        else if (number < 1000000)
        {
            return nameForNumber(number / 1000) + " thousand" + ((number % 1000 != 0) ? " " + nameForNumber(number % 1000) : "");
        }
        else if (number < 1000000000)
        {
            return nameForNumber(number / 1000000) + " million" + ((number % 1000000 != 0) ? " " + nameForNumber(number % 1000000) : "");
        }
        else if (number < 1000000000000)
        {
            return nameForNumber(number / 1000000000) + " billion" + ((number % 1000000000 != 0) ? " " + nameForNumber(number % 1000000000) : "");
        }
        return "big number";
    }

    static void replace(string& str, const string& from, const string& to)
    {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) < std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    static torch::Tensor prepare_text(string text)
    {
        if (!(text.back() == '.' || text.back() == '?' || text.back() == '!'))
        {
            text.append(".");
        }
        transform(text.begin(), text.end(), text.begin(), ::tolower);
        for (auto& abbreviation : abbreviations)
        {
            replace(text, get<0>(abbreviation), get<1>(abbreviation));
        }

        stringstream str_strm;
        str_strm << text;
        string temp_str;
        long temp_long;
        string from;
        size_t start_pos;
        while (!str_strm.eof())
        {
            str_strm >> temp_str;
            if (stringstream(temp_str) >> temp_long)
            {
                from = to_string(temp_long);
                start_pos = text.find(from);
                if(start_pos != std::string::npos)
                    text.replace(start_pos, from.length(), nameForNumber(temp_long));
            }
            temp_str.clear();
        }
        start_pos = 0;
        vector<string> stored_punctuations = {""};
        if (find(punctuations.begin(), punctuations.end(), text[0]) != punctuations.end())
        {
            stored_punctuations.back() = string(1, text[0]);
        }
        while ((start_pos = text.find(' ', start_pos)) != std::string::npos)
        {
            stored_punctuations.push_back(" ");
            if (find(punctuations.begin(), punctuations.end(), text[start_pos - 1]) != punctuations.end())
            {
                stored_punctuations.back().insert(0, 1, text[start_pos - 1]);
            }
            if (find(punctuations.begin(), punctuations.end(), text[start_pos + 1]) != punctuations.end())
            {
                stored_punctuations.back().push_back(text[start_pos + 1]);
            }
            start_pos++;
        }
        stored_punctuations.push_back("");
        if (find(punctuations.begin(), punctuations.end(), text.back()) != punctuations.end())
        {
            stored_punctuations.back() = string(1, text.back());
        }
        for (char punctuation : punctuations)
        {
            replace(text, string(1, punctuation), "");
        }
        espeak_Initialize(AUDIO_OUTPUT_SYNCH_PLAYBACK, 500, NULL, 0);
        espeak_SetVoiceByName("gmw/en-us");
        istringstream ss(text);
        string token;
        string new_text;
        while (getline(ss, token, ' '))
        {
            const char* text_c = token.c_str();
            const void** text_ptr = (const void**)&text_c;
            new_text.append(string(espeak_TextToPhonemes(text_ptr, 1, 24322)) + ' ');
        }
        text = new_text.substr(0, new_text.size() - 1);
        replace(text, "_", "");
        replace(text, "ˈ", "");
        const char* a = text.c_str();
        text.insert(0, stored_punctuations[0]);
        start_pos = 0;
        for (size_t i = 1; i < stored_punctuations.size() - 1; i++)
        {
            start_pos = text.find(" ", start_pos);
            text.replace(start_pos, 1, stored_punctuations[i]);
            start_pos += stored_punctuations[i].length();
        }
        text.append(stored_punctuations.back());
        vector<int> tokens = {};
        size_t current_phoneme;
        bool double_symbol = false;
        for (size_t i = 0; i < text.length(); i++)
        {
            if (double_symbol)
            {
                double_symbol = false;
                string double_symbol_text;
                double_symbol_text.push_back(text[i - 1]);
                double_symbol_text.push_back(text[i]);
                current_phoneme = find(all_phonemes.begin(), all_phonemes.end(), double_symbol_text) - all_phonemes.begin();
                tokens.push_back(current_phoneme);
                continue;
            }
            current_phoneme = find(all_phonemes.begin(), all_phonemes.end(), string(1, text[i])) - all_phonemes.begin();
            if (current_phoneme == all_phonemes.size())
            {
                double_symbol = true;
                continue;
            }
            tokens.push_back(current_phoneme);
        }

        return torch::tensor(tokens).unsqueeze(0);
    }

    static vector<unsigned short> ToWaveFile(const vector<float> &samples, const int sample_rate, const string &filename)
    {
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.channels = 1;
        format.sampleRate = sample_rate;
        format.bitsPerSample = 16;
        drwav* pWav = drwav_open_file_write(filename.c_str(), &format);

        std::vector<unsigned short> data;
        data.resize(samples.size());

        float max_value = std::fabs(samples[0]);
        for (size_t i = 0; i < samples.size(); i++)
        {
            max_value = std::max(max_value, std::fabs(samples[i]));
        }
        float factor = 32767.0f / std::max(0.01f, max_value);
        for (size_t i = 0; i < samples.size(); i++)
        {
            data[i] = std::max(uint16_t(0), std::min(std::numeric_limits<uint16_t>::max(), uint16_t(int(factor * samples[i]))));
        }

        drwav_uint64 n = static_cast<drwav_uint64>(samples.size());
        drwav_uint64 samples_written = drwav_write(pWav, n, data.data());
        drwav_close(pWav);
        return data;
    }

public:
    static vector<unsigned short> Generate(string text, string model_location = ".", string file_path = "")
    {
        torch::jit::Module glados = torch::jit::load("./glados.pt");
        torch::jit::Module vocoder = torch::jit::load("./vocoder-cpu-hq.pt");
        auto tts_input = prepare_text(text).to("cpu");
        auto tts_output = glados.get_method("generate_jit")({tts_input}).toGenericDict();
        auto mel = tts_output.at("mel_post").toTensor().to("cpu");
        auto audio_tensor = vocoder({mel}).toTensor().squeeze() * 32768.0;
        float* audio_array = audio_tensor.data_ptr<float>();
        size_t audio_size = audio_tensor.sizes()[0];
        vector<float> audio_vec;
        for (size_t i = 0; i < audio_size; i++)
        {
            audio_vec.push_back(audio_array[i]);
        }
        auto audio_data = ToWaveFile(audio_vec, 22050, "output.wav");
        return audio_data;
    }
};

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cout << "Usage: glados-tts \"text to read\"" << endl;
        return 1;
    }
    glados::Generate(argv[1], ".", "output.wav");
    return 0;
}
