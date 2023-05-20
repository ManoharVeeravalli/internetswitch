


class StringUtils {

public:
  static int indexOfReverse(String str, char c) {
    for (int i = str.length() - 1; i >= 0; i--) {
      if (str[i] == c) return i;
    }
    return -1;
  }
};