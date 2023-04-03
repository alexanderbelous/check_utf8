#include <bit>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace
{

// Returns true if the given byte is a UTF-8 continuation byte, false otherwise.
constexpr bool isContinuationByte(char8_t byte) noexcept
{
  constexpr char8_t kMask = static_cast<char8_t>(~0b00111111); // clears the lowest 6 bits.
  constexpr char8_t kExpected = 0b10000000;
  return (byte & kMask) == kExpected;
}

// If the given byte is a valid UTF-8 non-continuation byte, returns
// the number of continuation bytes that must follow it. Otherwise, returns -1.
constexpr int countContinuationBytes(char8_t first_byte) noexcept
{
  // Index of the highest zero bit of the input byte.
  // * 0 means that there are no zero bits.
  // * 1 means that only the least significant bit is zero.
  // * 8 means that the monst significant bit is zero.
  const int highest_zero_bit = std::bit_width(0xFFu & static_cast<unsigned int>(~first_byte));
  switch (highest_zero_bit)
  {
  case 8:      // 0xxxxxxx
    return 0;
  case 6:      // 110xxxxx
    return 1;
  case 5:      // 1110xxxx
    return 2;
  case 4:      // 11110xxx
    return 3;
  default:
    return -1;
  }
}

// Finds the first sequence of bytes that is not a valid UTF-8 sequence.
// Returns the offset of the first byte of the invalid sequence,
//         -1 if there is no such sequence.
std::ptrdiff_t findFirstNonUTF8Sequence(std::istream& data)
{
  std::ptrdiff_t offset = 0;
  char byte_current;
  // Read a byte while the stream is not empty.
  while(data.get(byte_current))
  {
    // Return the current position if the first byte is not a valid UTF-8 non-continuation byte.
    const int num_continuation_bytes = countContinuationBytes(static_cast<char8_t>(byte_current));
    if (num_continuation_bytes == -1)
    {
        return offset;
    }
    // Consume continuation bytes.
    for (int i = 0; i < num_continuation_bytes; ++i)
    {
        if (!data.get(byte_current) || !isContinuationByte(byte_current))
        {
            return offset;
        }
    }
    // So far so good.
    offset += (1 + num_continuation_bytes);
  }
  return -1;
}

}


int main(int argc, char** argv)
{
  if (argc != 2) {
    std::cout << "Usage: check_utf8 <input_path>" << std::endl;
    return 0;
  }
  const std::filesystem::path path_input(argv[1]);
  std::ifstream input(path_input, std::ios_base::binary);
  if (!input)
  {
    std::cerr << "Failed to open the file " << path_input;
    return -1;
  }
  const std::ptrdiff_t invalid_sequence_offset = findFirstNonUTF8Sequence(input);
  if (invalid_sequence_offset != -1)
  {
    std::cout << "Not UTF-8 sequence found at byte " << invalid_sequence_offset << std::endl;
    return -1;
  }
  std::cout << "The file is a valid UTF-8 text." << std::endl;
  return 0;
}
