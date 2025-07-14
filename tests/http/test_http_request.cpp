#include <iostream>
#include <cassert>
#include <string>
#include "../../src/http/http_request.hpp"

class HttpRequestTest {
public:
    void testBasicGetRequest() {
        std::cout << "Testing basic GET request..." << std::endl;
        
        std::string raw_request = 
            "GET /index.html HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "User-Agent: TestClient/1.0\r\n"
            "\r\n";
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.method == "GET");
        assert(request.path == "/index.html");
        assert(request.version == "HTTP/1.1");
        assert(request.headers["Host"] == "example.com");
        assert(request.headers["User-Agent"] == "TestClient/1.0");
        assert(request.body.empty());
        
        std::cout << "Basic GET request test passed!" << std::endl;
    }
    
    void testGetWithQueryParams() {
        std::cout << "Testing GET request with query parameters..." << std::endl;
        
        std::string raw_request = 
            "GET /search?q=hello+world&page=1&limit=10 HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n";
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.method == "GET");
        assert(request.path == "/search");
        assert(request.query_params["q"] == "hello world");
        assert(request.query_params["page"] == "1");
        assert(request.query_params["limit"] == "10");
        
        std::cout << "GET with query parameters test passed!" << std::endl;
    }
    
    void testPostWithBody() {
        std::cout << "Testing POST request with body..." << std::endl;
        
        std::string body_content = "{\"username\":\"test\",\"password\":\"123456\"}";
        std::string raw_request = 
            "POST /api/login HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body_content.length()) + "\r\n"
            "\r\n" +
            body_content;
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.method == "POST");
        assert(request.path == "/api/login");
        assert(request.headers["Host"] == "example.com");
        assert(request.headers["Content-Type"] == "application/json");
        assert(request.headers["Content-Length"] == std::to_string(body_content.length()));
        assert(request.body == body_content);
        
        std::cout << "POST with body test passed!" << std::endl;
    }
    
    void testUrlEncoding() {
        std::cout << "Testing URL encoding/decoding..." << std::endl;
        
        std::string raw_request = 
            "GET /search?q=hello%20world%21&special=%2B%26%3D HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n";
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.method == "GET");
        assert(request.path == "/search");
        assert(request.query_params["q"] == "hello world!");
        assert(request.query_params["special"] == "+&=");
        
        std::cout << "URL encoding test passed!" << std::endl;
    }
    
    void testEmptyBody() {
        std::cout << "Testing request with zero content length..." << std::endl;
        
        std::string raw_request = 
            "POST /api/ping HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.method == "POST");
        assert(request.path == "/api/ping");
        assert(request.headers["Content-Length"] == "0");
        assert(request.body.empty());
        
        std::cout << "Empty body test passed!" << std::endl;
    }
    
    void testHeadersWithSpaces() {
        std::cout << "Testing headers with various spacing..." << std::endl;
        
        std::string raw_request = 
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Authorization: Bearer token123\r\n"
            "X-Custom-Header:    value with spaces   \r\n"
            "\r\n";
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.headers["Host"] == "example.com");
        assert(request.headers["Authorization"] == "Bearer token123");
        assert(request.headers["X-Custom-Header"] == "value with spaces   ");
        
        std::cout << "Headers with spaces test passed!" << std::endl;
    }
    
    void testMalformedRequests() {
        std::cout << "Testing malformed requests..." << std::endl;
        
        // Test empty request
        auto empty_request = http::HttpRequest::parse("");
        assert(empty_request.method.empty());
        
        // Test invalid Content-Length
        std::string invalid_content_length = 
            "POST /api/test HTTP/1.1\r\n"
            "Content-Length: invalid\r\n"
            "\r\n"
            "some body";
        
        auto request_invalid_length = http::HttpRequest::parse(invalid_content_length);
        assert(request_invalid_length.method == "POST");
        assert(request_invalid_length.body.empty()); // Should not read body with invalid length
        
        std::cout << "Malformed requests test passed!" << std::endl;
    }
    
    void testMultilineBody() {
        std::cout << "Testing multiline body..." << std::endl;
        
        std::string body_content = "line1\nline2\r\nline3";
        std::string raw_request = 
            "POST /api/data HTTP/1.1\r\n"
            "Content-Length: " + std::to_string(body_content.length()) + "\r\n"
            "\r\n" +
            body_content;
        
        auto request = http::HttpRequest::parse(raw_request);
        
        assert(request.method == "POST");
        assert(request.path == "/api/data");
        assert(request.body == body_content);
        
        std::cout << "Multiline body test passed!" << std::endl;
    }
    
    void runAllTests() {
        try {
            testBasicGetRequest();
            testGetWithQueryParams();
            testPostWithBody();
            testUrlEncoding();
            testEmptyBody();
            testHeadersWithSpaces();
            testMalformedRequests();
            testMultilineBody();
            
            std::cout << "\n✅ All HttpRequest tests passed!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
            throw;
        }
    }
};

int main() {
    try {
        HttpRequestTest test;
        test.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed: " << e.what() << std::endl;
        return 1;
    }
}
