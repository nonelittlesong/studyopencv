//
// Created by song on 18-9-27.
//

#include <cwchar>
#include <clocale>
#include <cctype>
#include <utility>
#include <assert.h>
#include <android/log.h>
#include "CvText.h"

CvText::CvText(const char *fontName) {
    assert(fontName != nullptr);

    __android_log_print(ANDROID_LOG_DEBUG, "freetype", "构造函数开始");
    // open font library, create a font
    if (FT_Init_FreeType(&m_library)) throw;
    if (FT_New_Face(m_library, fontName, 0, &m_face)) throw;
    FT_Select_Charmap(m_face, FT_ENCODING_UNICODE);

    restoreFont();

    setlocale(LC_ALL, "zh_CN.utf8");
}

CvText::~CvText() {
    FT_Done_Face(m_face);
    FT_Done_FreeType(m_library);
}

// set font properties
void CvText::setFont(int *type, cv::Scalar *size, bool *underline, float *diaphaneity) {
    // check arguments validity
    if (type) {
        if (*type >= 0) m_fontType = *type;
    }
    if (size) {
        m_fontSize.val[0] = fabs(size->val[0]);
        m_fontSize.val[1] = fabs(size->val[1]);
        m_fontSize.val[2] = fabs(size->val[2]);
        m_fontSize.val[3] = fabs(size->val[3]);
    }
    if (underline) {
        m_fontUnderline = *underline;
    }
    if (diaphaneity) {
        m_fontDiaphaneity = *diaphaneity;
    }
}

// restore font
void CvText::restoreFont() {
    m_fontSize = 0;
    m_fontSize.val[0] = 15;
    m_fontSize.val[1] = 0.5;
    m_fontSize.val[2] = 0.1;
    m_fontSize.val[3] = 0;

    m_fontUnderline = false;
    m_fontDiaphaneity = 1.0;

    // character size
    FT_Set_Pixel_Sizes(m_face, (FT_UInt) m_fontSize.val[0], 0);
}

int CvText::putText(cv::Mat &frame, std::string text, cv::Point pos, cv::Scalar color) {
    return putText(frame, text.c_str(), pos, std::move(color));
}
int CvText::putText(cv::Mat &frame, const char *text, cv::Point pos, cv::Scalar color) {
    if (frame.empty()) return -1;
    if (text == nullptr) return -2;

    wchar_t *w_str;
    int count = char2Wchar(text, w_str);

    int i = 0;
    for (; i < count; ++i) {
        wchar_t wc = w_str[i];
        if (wc < 128)
            FT_Set_Pixel_Sizes(m_face, (FT_UInt)(m_fontSize.val[0]*1.15), 0);
        else
            FT_Set_Pixel_Sizes(m_face, (FT_UInt)m_fontSize.val[0], 0);
        putWChar(frame, wc, pos, color);
    }

    delete(w_str);
    return i;
}

int CvText::char2Wchar(const char *&src, wchar_t *&dst, const char *locale) {
    if (src == nullptr) {
        dst = nullptr;
        return 0;
    }

    setlocale(LC_CTYPE, locale);

    int w_size = (int)mbstowcs(nullptr, src, 0) + 1;

    if (w_size == 0) {
        dst = nullptr;
        return -1;
    }

    dst = new wchar_t[w_size];

    auto ret = (int)mbstowcs(dst, src, strlen(src) + 1);
    if  (ret <= 0) return -1;

    return ret;
}

void CvText::putWChar(cv::Mat &frame, wchar_t wc, cv::Point &pos, cv::Scalar color) {
    IplImage img = IplImage(frame);

    FT_UInt glyph_index = FT_Get_Char_Index(m_face, (FT_ULong)wc);
    FT_Load_Glyph(m_face, glyph_index, FT_LOAD_DEFAULT);
    FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_MONO);

    FT_GlyphSlot slot = m_face->glyph;

    int rows = slot->bitmap.rows;
    int cols = slot->bitmap.width;

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            int off = ((img.origin == 0) ? i : (rows - 1 - i)) * slot->bitmap.pitch + j / 8;

            if (slot->bitmap.buffer[off] & (0xC0 >> (j % 8))) {
                int r = (img.origin == 0) ? pos.y - (rows - 1 - i) : pos.y + i;
                int c = pos.x + j;

                if (r >= 0 && r < img.height && c >= 0 && c <img.width) {
                    CvScalar scalar = cvGet2D(&img, r, c);

                    float p = m_fontDiaphaneity;
                    for (int k = 0; k < 4; ++k) {
                        scalar.val[k] = scalar.val[k] * (1 - p) + color.val[k] * p;
                    }
                    cvSet2D(&img, r, c, scalar);
                }
            }
        }
    }

    double space = m_fontSize.val[0] * m_fontSize.val[1];
    double sep = m_fontSize.val[0] * m_fontSize.val[2];

    pos.x += (int)((cols ? cols : space) + sep);
}