#include "htmlcxx/html/ParserDom.h"
#include <curl/curl.h>
#include <cstdio>
#include <cstdlib>
#include <htmlcxx/html/ParserDom.h>
#include <iostream>
#include <list>
#include <pthread.h>
#include <queue>
#include <set>
#include <string>
#include <unistd.h>

using namespace std;
using namespace htmlcxx;

#define MASTER_URL "http://cse.iitkgp.ac.in"
#define SPIDER_COUNT 5

typedef struct urlstruct {

	string urlname;
	pthread_t thread_id;
	int depth;
}URL;

size_t write_to_string(void*,size_t,size_t,void*);
int spawn_threads();
void* crawl(void*);
int level_update();
int extract_URLs(URL&,queue<URL>&);
int print_result();

int final_level;
pthread_t spiders[SPIDER_COUNT];
set<string> url_list;

int level;
pthread_mutex_t level_mutex;
queue<URL> to_do;
pthread_mutex_t to_do_mutex;
queue<URL> done_list;
pthread_mutex_t done_mutex;
queue<URL> to_do_next;
pthread_mutex_t to_do_next_mutex;

int waiting_threads;
pthread_mutex_t waiting_threads_mutex;

pthread_cond_t condition;
pthread_mutex_t condition_mutex;

int main(void){

	URL master;
	pthread_mutex_init(&level_mutex, NULL);
	pthread_mutex_init(&to_do_mutex, NULL);
	pthread_mutex_init(&done_mutex, NULL);
	pthread_mutex_init(&to_do_next_mutex, NULL);
	pthread_mutex_init(&waiting_threads_mutex, NULL);

	fscanf(stdin,"%10d",&final_level);

	level = 1;
	waiting_threads = 0;
	pthread_cond_init (&condition, NULL);

	master.urlname = MASTER_URL;
	master.thread_id = pthread_self();
	master.depth = 1;

	to_do.push(master);

	spawn_threads();
	print_result();

	pthread_cond_destroy(&condition);
	pthread_mutex_destroy(&level_mutex);
	pthread_mutex_destroy(&to_do_mutex);
	pthread_mutex_destroy(&done_mutex);
	pthread_mutex_destroy(&to_do_next_mutex);
	pthread_mutex_destroy(&waiting_threads_mutex);

	pthread_exit(NULL);
	return 0;
}

int spawn_threads(){

	for(int i = 0;i < SPIDER_COUNT; i++){
		if(pthread_create(&spiders[i],NULL,&crawl,NULL)){
			perror("thread spawned");
			exit(1);
		}
	}
	for(int i = 0;i < SPIDER_COUNT; i++){
		pthread_join(spiders[i], NULL);
	}

	return 0;
}

void* crawl(void*){

	while(1){
		URL current_URL;

		pthread_mutex_lock(&to_do_mutex);
		if(to_do.empty()) {
			cout <<"Thread " <<pthread_self() << " found TODO queue empty\n";
			pthread_mutex_unlock(&to_do_mutex);
			level_update();
		} else {
			current_URL = to_do.front();
			to_do.pop();
			cout <<"Thread " <<pthread_self() << " : " <<current_URL.urlname <<"\n";
			pthread_mutex_unlock(&to_do_mutex);

			pthread_mutex_lock(&done_mutex);
			if(url_list.find(current_URL.urlname) != url_list.end()) {
				pthread_mutex_unlock(&done_mutex);
				continue;
			}
			else
				pthread_mutex_unlock(&done_mutex);

			extract_URLs(current_URL,to_do_next);

			pthread_mutex_lock(&done_mutex);
			{
				url_list.insert(current_URL.urlname);
				done_list.push(current_URL);
			}
			pthread_mutex_unlock(&done_mutex);
		}
	}
	return NULL;
}

int level_update(){

	pthread_mutex_lock(&waiting_threads_mutex);
	waiting_threads++;

	if (waiting_threads < 5) {
		pthread_mutex_unlock(&waiting_threads_mutex);
		pthread_cond_wait (&condition, &condition_mutex);
		cout <<"Thread " <<pthread_self() <<" woke up\n";
	} else {
		waiting_threads = 0;
		pthread_mutex_unlock(&waiting_threads_mutex);
		level++;
		while(!to_do_next.empty()){
			to_do.push(to_do_next.front());
			to_do_next.pop();
		}
		pthread_cond_broadcast(&condition);
	}
	pthread_mutex_lock(&level_mutex);
	if (level > final_level) {
		pthread_mutex_unlock(&level_mutex);
		cout <<"Thread " <<pthread_self() << " exiting\n";
		pthread_exit(NULL);
	}
	else
		pthread_mutex_unlock(&level_mutex);

	// if(level < final_level){
	// 	level++;
	// 	while(!to_do_next.empty()){
	// 		to_do.push(to_do_next.front());
	// 		to_do_next.pop();
	// 	}
	// }
	// else{
	// 	pthread_exit(NULL);
	// }

	return 0;
}

int extract_URLs(URL& url,queue<URL>& queue){


	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.urlname.c_str());

		std::string response;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

    // The "response" variable should now contain the contents of the HTTP response
		HTML::ParserDom parser;
		tree<HTML::Node> dom = parser.parseTree(response);

  //Dump all links in the tree
		tree<HTML::Node>::iterator it = dom.begin();
		tree<HTML::Node>::iterator end = dom.end();
		for (; it != end; ++it){
			if (it->tagName() == "a"){
				it->parseAttributes();
				std::map<std::string, std::string> pairs = it->attributes();
				for ( std::map<std::string, std::string>::const_iterator iter = pairs.begin();
					iter != pairs.end(); ++iter ) {
					if(iter->first.compare("href") == 0){
						string httplink("http://");
						if (std::mismatch(httplink.begin(), httplink.end(), iter->second.begin()).first == httplink.end())
						{
							URL newURL;
							newURL.urlname = iter->second;
							newURL.thread_id = pthread_self();
							newURL.depth = level;
							pthread_mutex_lock(&to_do_next_mutex);
							{
								queue.push(newURL);
							}
							pthread_mutex_unlock(&to_do_next_mutex);
						}
					}
				}
			}
		}
	}

	return 0;
}

size_t write_to_string(void *ptr, size_t size, size_t count, void *stream) {
	((std::string*)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

int print_result(){
	while(done_list.empty()){
		fprintf(stdout,"%30s %6ld %3d",done_list.front().urlname.c_str(),done_list.front().thread_id,done_list.front().depth);
		done_list.pop();
	}
	return 0;
}
