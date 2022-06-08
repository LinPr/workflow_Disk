#include "json.hpp"
#include "Token.h"
#include <unix.h>
#include <iostream>
#include <vector>
#include <iostream>
#include <map>
#include <workflow/WFTaskFactory.h>
#include <workflow/WFFacilities.h>
#include <workflow/MySQLResult.h>
#include <wfrest/HttpServer.h>
#include <wfrest/FileUtil.h>



using Json = nlohmann::json;

static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int num){
    waitGroup.done();
    printf("waitGroup is done\n");
}

int main()
{

    signal(SIGINT,sigHandler);
    wfrest::HttpServer server;
    

    /* 1. 用户注册接口 */
    server.GET("/user/signup", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/signup.html");
    });
    server.POST("/user/signup", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        std::string salt = "abcdefgh";
        std::map<std::string,std::string> form_KV = req->form_kv();
        std::string username = form_KV["username"];
        std::string password = form_KV["password"];
        std::string encryptPassword = crypt(password.c_str(), salt.c_str());
        std::cout << "username = " << username << std::endl
                  << "password = " << password << std::endl
                  << "encryptPassword = " << encryptPassword << std::endl;

        /* 记录新注册的用户 */     
        std::string sql = "INSERT INTO wfrest.tbl_user (`user_name`,`user_pwd`) VALUES('" 
            + username + "','" + encryptPassword + "');";
        std::cout << sql << std::endl;

        resp->MySQL("mysql://root:321lpr.@127.0.0.1", sql, [resp](Json *json){
            // std::cout << json->dump() << std::endl;
            /* 判断SQL语句是否执行成功 */
            if((*json)["status"].is_null()){ resp->String("NO");  return -1; }
            else if((*json)["status"] == "OK"){ resp->String("SUCCESS"); return -1; }
            else{ resp->String("NO"); return -1; }
        });
    });
   

    /* 2. 用户登录接口 */
    server.GET("/user/signin", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/signin.html");
    });
    server.GET("/static/view/signin.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/signin.html");
    });
    server.POST("/user/signin",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        std::map<std::string,std::string> form_KV = req->form_kv();
        std::string username = form_KV["username"];
        std::string password = form_KV["password"];
        std::string salt = "abcdefgh";
        std::string encryptPassword = crypt(password.c_str(), salt.c_str());
        
        std::string sql = "SELECT user_pwd from wfrest.tbl_user WHERE user_name = '" + username + "' LIMIT 1";
        // std::cout << sql << std::endl;
        resp->MySQL("mysql://root:321lpr.@127.0.0.1", sql, [resp, username, encryptPassword](Json *json){
            // std::cout << json->dump() << std::endl;

            /* 判断密码的哈希是否和数据库中的哈希相等 */
            std::string correctPassword = (*json)["result_set"][0]["rows"][0][0];
            if(correctPassword != encryptPassword){
                resp->String("FAIL");
                return -1;
            }

            /* 用户每次登录都重新生成新的 Token */
            Token userToken(username, "abcdefgh");
            std::cout << userToken.get_token() << std::endl;
            std::string sql = "REPLACE INTO wfrest.tbl_user_token (`user_name`,`user_token`) VALUES ('"
                + username + "', '" + userToken.get_token() + "');" ;
            std::cout << sql << std::endl;
            
            resp->MySQL("mysql://root:321lpr.@localhost",sql,[username,userToken,resp](Json *json){
                if((*json)["status"] != "OK"){  resp->String("FAIL");  return -1; }
                std::cout << "OK" << std::endl;

                /* 把 Token 返回给客户端 */
                Json data;
                data["Token"] = userToken.get_token();
                data["Username"] = username;
                data["Location"] = "/static/view/home.html";
                Json respJson;
                respJson["data"] = data;
                respJson["msg"] = "OK";
                respJson["code"] = 0;
                std::cout << respJson.dump() << std::endl;
                resp->String(respJson.dump());
            });
        });
    });


    /* 3. 显示网盘主页 */
    server.GET("/static/view/home.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/home.html");
    });
    server.GET("/static/img/avatar.jpeg",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/img/avatar2.jpeg");
    });
    server.GET("/static/js/auth.js",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/js/auth.js");
    });

    /* 4. 向服务器数据库请求并取出该用户对应的数信息 */
    server.POST("/user/info",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        std::map<std::string,std::string> userInfo = req->query_list();
        std::string sql = "SELECT user_name,signup_at from wfrest.tbl_user WHERE user_name = '"
            + userInfo["username"] + "' LIMIT 1;";
        resp->MySQL("mysql://root:321lpr.@localhost",sql,[resp](Json *json){
            Json data;
            data["Username"] = (*json)["result_set"][0]["rows"][0][0];
            data["SignupAt"] = (*json)["result_set"][0]["rows"][0][1];
            Json respJson;
            respJson["code"] = 0;
            respJson["msg"] = "OK";
            respJson["data"] = data;
            // std::cout << respJson.dump() << std::endl;

            /* 将该用户的相关数据返回客户端显示 */
            resp->String(respJson.dump());
        });
    });

    /* 5. 从数据库中取出该用户对应存储的文件信息 */
    server.POST("/user/info", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        
        // 1、解析前端发送过来的信息
        std::map<std::string,std::string> userInfo = req->query_list();
        std::string username = userInfo["username"];
        std::map<std::string,std::string> form_KV = req->form_kv();
        std::string limit = form_KV["limit"];

        // 2、根据用户名，去查找tbl_user_file
        std::string sql = "SELECT file_sha1,file_name,file_size,upload_at,last_update FROM cloudisk.tbl_user_file WHERE ";
        sql = sql + "user_name ='" + username + "' LIMIT " + limit;
        std::cout << sql << std::endl;
        resp->MySQL("mysql://root:321lpr.@localhost",sql,[resp](Json *json){
            int rowCount = (*json)["result_set"][0]["rows_count"];
            Json jsonArr;
            for(int i = 0; i < rowCount; ++i){
                Json jsonEle;
                jsonEle["FileHash"] = (*json)["result_set"][0]["rows"][i][0];
                jsonEle["FileName"] = (*json)["result_set"][0]["rows"][i][1];
                jsonEle["FileSize"] = (*json)["result_set"][0]["rows"][i][2];
                jsonEle["UploadAt"] = (*json)["result_set"][0]["rows"][i][3];
                jsonEle["LastUpdated"] = (*json)["result_set"][0]["rows"][i][4];
                jsonArr.push_back(jsonEle);
            }
            //向客户端显示
            resp->String(jsonArr.dump());
        });

    });

    /* 6. 上传文件接口 */

    /* 7. 下载文件接口 */





    if(server.track().start(1234) == 0){
    server.list_routes();
    waitGroup.wait();
    server.stop();
    }
    else{
        std::cerr << "Cannot start server" <<std::endl;
        exit(1);
    }
    return 0;
}