// #include "httpClient.h"
// #include "lwip/netdb.h"
// #include "mbedtls/net.h"
// #include "freertos/FreeRTOS.h"
// #include <lwip/dns.h>
// #include "mbedtls/entropy.h"
// #include "mbedtls/ctr_drbg.h"
// #include <string.h>
// //#include <stdlib.h>
// #include <stdio.h>

// #include "wifi.h"
// #include "esp_log.h"

// #define DNS_IP "8.8.8.8"

// //#define DEBUG_ME
// //#define error

// /* Root cert for howsmyssl.com, stored in cert.c */

// static const char *TAG = "httpClient";


// ip_addr_t host_ip;
// mbedtls_entropy_context entropy;
// mbedtls_ctr_drbg_context ctr_drbg;
// mbedtls_ssl_context ssl;
// mbedtls_x509_crt cacert;
// mbedtls_ssl_config conf;
// mbedtls_net_context server_fd;
// extern const char *server_root_cert;

// //const int buf_len=1024;



// //struct addrinfo *res;
// bool ranFirstTimeInit = false;

// bool UPDATE =true;

// int firstTimeInit()
// {
// 	if(ranFirstTimeInit)
// 		return 0;

// 	mbedtls_x509_crt_init(&cacert);

// 	int ret = mbedtls_x509_crt_parse(&cacert, (uint8_t*)server_root_cert, strlen(server_root_cert)+1);
// 	if(ret < 0)
// 	{
// 		#if defined (DEBUG_ME)
// 		printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
// 		#endif
// 		return -2;
// 	}
// 	//mbedtls_x509_crt_free(&cacert); IN CASE you need to free things
// 	ranFirstTimeInit = true;
// 	return 0;

// }

// int httpInit()
// {

// 	int ret;
// 	//printf("HTTPint\n");
// 	const char *pers = "ssl_client1";

// 	/*
// 	 * 0. Initialize the RNG and the session data
// 	 */
// 	if(firstTimeInit()<0)
// 		return -1;
// 	mbedtls_ssl_init(&ssl);
// 	mbedtls_ctr_drbg_init(&ctr_drbg);
// 	//printf("\n. Seeding the random number generator...");

// 	mbedtls_ssl_config_init(&conf);

// 	mbedtls_entropy_init(&entropy);
// 	if((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
// 									(const unsigned char *) pers,
// 									strlen(pers))) != 0)
// 	{
// 		#if defined (DEBUG_ME)
// 		printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
// 		#endif
// 		return -1;
// 	}

// 	//printf(" ok\n");
// 	//printf("top of loop, free heap = %u\n", xPortGetFreeHeapSize());
// 	/*
// 	 * 0. Initialize certificates
// 	 */
// 	//printf("  . Loading the CA root certificate ...");

// 	//printf(" ok (%d skipped)\n", ret);
// 	//printf("top of loop, free heap = %u\n", xPortGetFreeHeapSize());
// 	/* Hostname set here should match CN in server certificate */
// 	if((ret = mbedtls_ssl_set_hostname(&ssl, WEB_SERVER)) != 0)
// 	{
// 		#if defined (DEBUG_ME)
// 		printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
// 		#endif
// 		return -3;
// 	}

// 	/*
// 	 * 2. Setup stuff
// 	 */
// 	 //printf("top of loop, free heap = %u\n", xPortGetFreeHeapSize());
// 	//printf("  . Setting up the SSL/TLS structure...");

// 	if((ret = mbedtls_ssl_config_defaults(&conf,
// 										MBEDTLS_SSL_IS_CLIENT,
// 										MBEDTLS_SSL_TRANSPORT_STREAM,
// 										MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
// 	{
// 		 #if defined (DEBUG_ME)
// 		printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
// 		#endif
// 		return -4;
// 	}

// 	//printf(" ok\n");

// 	/* OPTIONAL is not optimal for security, in this example it will print
// 	   a warning if CA verification fails but it will continue to connect.
// 	*/

// 	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
// 	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
// 	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

// 	if((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
// 	{
// 		return -5;
// 	}

// 	return 1;



// }

// int serverConnect()
// {

// 	int flags,ret;
// 	mbedtls_net_init(&server_fd);

// 		int opt = 5000;
// 		lwip_setsockopt(server_fd.fd, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));
// 		int optval = 1;
// 		lwip_setsockopt(server_fd.fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));

// 		if((ret = mbedtls_net_connect(&server_fd, WEB_SERVER,
// 									 	WEB_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
// 		{
// 			#if defined (DEBUG_ME)
// 			printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
// 			#endif

// 			return -1;
// 		}

// 		mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

// 	mbedtls_ssl_conf_read_timeout(&conf,20000);

// 		/*
// 		 * 4. Handshake
// 		 */

// 		while((ret = mbedtls_ssl_handshake(&ssl)) != 0)
// 		{
// 			if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
// 			{
// 				 #if defined (DEBUG_ME)

// 				printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
// 				#endif

// 				return -2;
// 			}
// 		}



// 		/*
// 		 * 5. Verify the server certificate
// 		 */


// 		/* In real life, we probably want to bail out when ret != 0 */
// 		if((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
// 		{
// 			#if defined (DEBUG_ME)
// 			char vrfy_buf[512];
// 			printf(" failed234\n");

// 			mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
// 			printf("%s\n", vrfy_buf);
// 			#endif

// 		}
// 		else
// 	 	{
// 			#if defined (DEBUG_ME)
// 			printf(" ok\n");
// 			#endif
// 		}


// 		return 1;

// }

// int writeData(char *data,int len)
// {
// 	int ret;
// 	//printf("%s",data);
// 	while((ret = mbedtls_ssl_write(&ssl, (const unsigned char*)data, len)) <= 0)
// 	{
// 		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
// 		{
// 			// printf(" failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);

// 			ESP_LOGE(TAG, " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret);

// 			return -1;
// 		}
// 		else
// 		{
// 				ESP_LOGE(TAG, "X Some error in writing X\n");
// 		}
// 	}
// 	//printf("Wrote %d dlen %d \n", ret,len);
// 	return 0;

// }



// int read_ssl_Data(unsigned char **buf,int len)
// {
// 	int ret;
// 	//len = buf_len;
// 	ret = mbedtls_ssl_read(&ssl, *buf, len*sizeof(char));
// 	#if defined (DEBUG_ME)
// 	printf("\nReceive From ServerReturn code %d\n",ret );
// 	#endif

// 	#if defined (DEBUG_ME)
// 	if(ret<0)
// 	{
// 		printf("failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
// 		return ret;
// 	}
// 	#endif

// 	if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
// 	{

// 	}
// 		//continue;

// 	if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
// 		ret = 0;
// 		//break;
// 	}

// 	if(ret < 0)
// 	{
// 		#if defined (DEBUG_ME)
// 		printf("failed\n  ! mbedtls_ssl_read returned %d\n\n", ret);
// 		#endif
// 		return ret;
// 	}

// 	if(ret == 0)
// 	{
// 		//printf("\n\nEOF\n\n");
// 		return -3;
// 	}

// 	//len = ret;
// 	//buf[len]='\0';
// 	return ret;
// }



// void superclose()
// {
// 	//startHttpParsing();
// 	//mbedtls_ssl_session_reset(&ssl);
// 	mbedtls_net_free( &server_fd );
// 	mbedtls_ssl_free( &ssl );
// 	mbedtls_ssl_config_free( &conf );
// 	mbedtls_ctr_drbg_free( &ctr_drbg );
// 	mbedtls_entropy_free( &entropy );


// }
// void Httpinitclose()

// {
// 	mbedtls_ssl_session_reset(&ssl);
// 	mbedtls_ssl_free( &ssl );
// 	mbedtls_ssl_config_free( &conf );
// 	mbedtls_ctr_drbg_free( &ctr_drbg );
// 	mbedtls_entropy_free( &entropy );

// }
// bool isThereSomethingToRead(int timeout_ms)
// {
// 	int my_socket = server_fd.fd;
// 	struct timeval tv;
// 	fd_set fdset;
// 	int rc = 0;
// 	FD_ZERO(&fdset);
// 	FD_SET(server_fd.fd, &fdset);
// 	tv.tv_sec = 0;
// 	tv.tv_usec = timeout_ms*1000;
// 	rc = select(server_fd.fd + 1, &fdset, 0, 0, &tv);
// 	if ((rc > 0) && (FD_ISSET(my_socket, &fdset)))
// 	{
// 		return true;
// 	}
// 	else
// 	{
// 		// select fail
// 		return false;
// 	}
// }
// bool isOkToWrite(int timeout_ms)
// {
// 	int my_socket = server_fd.fd;
// 	struct timeval tv;
// 	fd_set fdset;
// 	int rc = 0;
// 	FD_ZERO(&fdset);
// 	FD_SET(server_fd.fd, &fdset);
// 	tv.tv_sec = 0;
// 	tv.tv_usec = timeout_ms*1000 / portTICK_PERIOD_MS;
// 	rc = select(server_fd.fd + 1, 0, &fdset, 0, &tv);
// 	if ((rc > 0) && (FD_ISSET(my_socket, &fdset)))
// 	{
// 		return true;
// 	}
// 	else
// 	{
// 		// select fail
// 		return false;
// 	}
// }

// void setSocketTimeout(int timeout){
// 	int opt = timeout;
// 	lwip_setsockopt(server_fd.fd, SOL_SOCKET, SO_RCVTIMEO, &opt, sizeof(int));
// }


// int connectionCheack()
// {
// 	if(wifi_sta_get_connected_status()!=true){
// 		//Printwarning("calling SOFT:restart ");
// 		//restart();
// 		return -1;
// 	}
// 	const struct addrinfo hints = {
// 		.ai_family = AF_INET,
// 		.ai_socktype = SOCK_STREAM,
// 	};
// 	struct addrinfo *res;

// 	ip4_addr_t dns_ip_v4;
// 	ip4addr_aton(DNS_IP, &dns_ip_v4);
// 	ip_addr_t dns_ip = IPADDR4_INIT(dns_ip_v4.addr);
// 	dns_setserver(0, &dns_ip);

// 	if( getaddrinfo(WEB_SERVER,WEB_PORT, &hints,&res) != ERR_OK)
// 	{
// 		ESP_LOGE(TAG, "DNS lookup Failed");
// 		freeaddrinfo(res);
// 		return INTERNET_CONNECT_FAILED;
// 	}
// 	ESP_LOGI(TAG, "DNS lookup succeeded");
// 	freeaddrinfo(res);
// 	return INTERNET_CONNECTED;
// }




