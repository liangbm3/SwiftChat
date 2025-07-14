#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "../../src/http/http_response.hpp"

class HttpResponseTest {
public:
    void testBasicResponse() {
        std::cout << "Testing basic response..." << std::endl;
        
        http::HttpResponse response(200, "Hello World");
        
        assert(response.status_code == 200);
        assert(response.body == "{\"message\":\"Hello World\"}");
        assert(response.headers["Content-Type"] == "application/json; charset=utf-8");
        assert(response.headers["Server"] == "SwiftChat/1.0");
        assert(!response.headers["Date"].empty());
        assert(response.headers["Connection"] == "close");
        
        std::cout << "Basic response test passed!" << std::endl;
    }
    
    void testJsonResponse() {
        std::cout << "Testing JSON response..." << std::endl;
        
        std::string json_body = "{\"username\":\"test\",\"status\":\"online\"}";
        http::HttpResponse response(200, json_body);
        
        assert(response.status_code == 200);
        assert(response.body == json_body);
        assert(response.headers["Content-Type"] == "application/json; charset=utf-8");
        
        std::cout << "JSON response test passed!" << std::endl;
    }
    
    void testArrayJsonResponse() {
        std::cout << "Testing JSON array response..." << std::endl;
        
        std::string json_array = "[{\"id\":1,\"name\":\"test\"},{\"id\":2,\"name\":\"test2\"}]";
        http::HttpResponse response(200, json_array);
        
        assert(response.status_code == 200);
        assert(response.body == json_array);
        assert(response.headers["Content-Type"] == "application/json; charset=utf-8");
        
        std::cout << "JSON array response test passed!" << std::endl;
    }
    
    void testEmptyResponse() {
        std::cout << "Testing empty response..." << std::endl;
        
        http::HttpResponse response(204, "");
        
        assert(response.status_code == 204);
        assert(response.body.empty());
        assert(response.headers["Content-Type"] == "text/plain");
        
        std::cout << "Empty response test passed!" << std::endl;
    }
    
    void testSpecialCharacters() {
        std::cout << "Testing special characters..." << std::endl;
        
        std::string message_with_quotes = "He said \"Hello\" and she replied.";
        http::HttpResponse response(200, message_with_quotes);
        
        assert(response.status_code == 200);
        // 检查特殊字符是否被正确转义
        assert(response.body.find("\\\"") != std::string::npos);
        assert(response.headers["Content-Type"] == "application/json; charset=utf-8");
        
        std::cout << "Special characters test passed!" << std::endl;
    }
    
    void testNewlineAndControlCharacters() {
        std::cout << "Testing newline and control characters..." << std::endl;
        
        std::string message_with_newline = "Line 1\nLine 2\tTabbed\rCarriage Return";
        http::HttpResponse response(200, message_with_newline);
        
        assert(response.status_code == 200);
        // 检查换行符、制表符等是否被正确转义
        assert(response.body.find("\\n") != std::string::npos);
        assert(response.body.find("\\t") != std::string::npos);
        assert(response.body.find("\\r") != std::string::npos);
        
        std::cout << "Newline and control characters test passed!" << std::endl;
    }
    
    void testToString() {
        std::cout << "Testing toString method..." << std::endl;
        
        http::HttpResponse response(404, "Not Found");
        std::string response_string = response.toString();
        
        // 检查响应字符串格式
        assert(response_string.find("HTTP/1.1 404 Not Found") != std::string::npos);
        assert(response_string.find("Content-Length:") != std::string::npos);
        assert(response_string.find("Content-Type: application/json; charset=utf-8") != std::string::npos);
        assert(response_string.find("Server: SwiftChat/1.0") != std::string::npos);
        assert(response_string.find("\r\n\r\n") != std::string::npos); // 头部和体之间的空行
        
        std::cout << "toString test passed!" << std::endl;
    }
    
    void testStatusCodes() {
        std::cout << "Testing various status codes..." << std::endl;
        
        struct TestCase {
            int code;
            std::string expected_text;
        };
        
        std::vector<TestCase> test_cases = {
            {200, "OK"},
            {201, "Created"},
            {302, "Found"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {409, "Conflict"},
            {500, "Internal Server Error"},
            {999, "Unknown"}
        };
        
        for (const auto& test_case : test_cases) {
            http::HttpResponse response(test_case.code, "test");
            std::string response_string = response.toString();
            
            std::string expected_status_line = "HTTP/1.1 " + std::to_string(test_case.code) + " " + test_case.expected_text;
            assert(response_string.find(expected_status_line) != std::string::npos);
        }
        
        std::cout << "Status codes test passed!" << std::endl;
    }
    
    void testContentLength() {
        std::cout << "Testing content length calculation..." << std::endl;
        
        http::HttpResponse response(200, "Test message");
        std::string response_string = response.toString();
        
        // 计算实际body长度
        size_t expected_length = response.body.length();
        std::string expected_content_length = "Content-Length: " + std::to_string(expected_length);
        
        assert(response_string.find(expected_content_length) != std::string::npos);
        
        std::cout << "Content length test passed!" << std::endl;
    }
    
    void testCustomHeaders() {
        std::cout << "Testing custom headers..." << std::endl;
        
        http::HttpResponse response(200, "Custom response");
        response.headers["X-Custom-Header"] = "custom-value";
        response.headers["Cache-Control"] = "no-cache";
        
        std::string response_string = response.toString();
        
        assert(response_string.find("X-Custom-Header: custom-value") != std::string::npos);
        assert(response_string.find("Cache-Control: no-cache") != std::string::npos);
        
        std::cout << "Custom headers test passed!" << std::endl;
    }
    
    void runAllTests() {
        try {
            testBasicResponse();
            testJsonResponse();
            testArrayJsonResponse();
            testEmptyResponse();
            testSpecialCharacters();
            testNewlineAndControlCharacters();
            testToString();
            testStatusCodes();
            testContentLength();
            testCustomHeaders();
            
            std::cout << "\n✅ All HttpResponse tests passed!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
            throw;
        }
    }
};

int main() {
    try {
        HttpResponseTest test;
        test.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed: " << e.what() << std::endl;
        return 1;
    }
}
