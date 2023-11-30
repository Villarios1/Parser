#include <string>
#include "curl/curl.h"
#include "gumbo.h"
#include <algorithm>
#include <iostream>
#include <fstream>

std::string request(std::string& url)
{
    CURLcode res_code = CURLE_FAILED_INIT;
    CURL* curl = curl_easy_init();
    std::string result;

    curl_global_init(CURL_GLOBAL_ALL);
    typedef size_t(*curl_write)(char*, size_t, size_t, std::string*);
    if (curl)
    {
        curl_easy_setopt(curl,
            CURLOPT_WRITEFUNCTION,
            static_cast<curl_write> ([](char* contents, size_t size,
                size_t nmemb, std::string* data) -> size_t {
                    size_t new_size = size * nmemb;
                    if (data == nullptr)
                    {
                        return 0;
                    }
                    data->append(contents, new_size);
                    return new_size;
                }));
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "easy scrapper");

        res_code = curl_easy_perform(curl);

        if (res_code != CURLE_OK)
        {
            return curl_easy_strerror(res_code);
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return result;
}

std::string extract_topizdato(GumboNode* node)
{
    if (node->type == GUMBO_NODE_TEXT) // условие дл€ возврата из рекурсии
    {
        return std::string(node->v.text.text); // возвращаем извлеченные части
    }
    else if (node->type == GUMBO_NODE_ELEMENT)
    {
        std::string contents = "";
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            std::string text = extract_topizdato(static_cast<GumboNode*>(children->data[i])); // собираем извлеченные части в одну строку
            contents.append(text);
        }
        return contents; // и возвращаем результат
    }
    else
    {
        return "";
    }
}

// вызываетс€ тыс€чи раз
std::string find_definitions(GumboNode* node)
{
    std::string search_for{ "Developer" };
    std::string res;
    if (node->type != GUMBO_NODE_ELEMENT) // ищем элемент
    {
        res = "";
    }
    else if (node->v.element.tag == GUMBO_TAG_LI) // с тэгом <li>
    {
        res += extract_topizdato(node); // забираем из него текст
        if (res.find(search_for) == std::string::npos)
            res.clear();
        else
            res += "\n";
    }
    else
    {
        GumboVector* children = &node->v.element.children;
        for (int i = 0; i < children->length; ++i)
        {
            res += find_definitions(static_cast<GumboNode*>(children->data[i])); // рекурсивно вызываем потомков
        }
    }
    return res;
}

std::string scrape(std::string_view markup)
{
    GumboOutput* output = gumbo_parse_with_options(&kGumboDefaultOptions, markup.data(), markup.length()); // создает из полученной страницы GumboOutput

    std::string res{ find_definitions(output->root) }; // и отправл€ем его в ф. –езультат которой возвращаем.

    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return res;
}

int main()
{
    std::string url{ "https://topizda.to/" };

    std::string text{ request(url) }; // получаем текстовое представление страницы

    //std::cout << scrape(text) << std::endl; // консоль не поймет кириллицу в utf-8
    std::string scrapped{ scrape(text) };
    std::ofstream os("test.txt"); // запишем результат в файл
    if (!os)
    {
        std::cout << "Failed to open file" << '\n';
        exit(1);
    }
    os << scrapped;
    os.close();

    return EXIT_SUCCESS;
}