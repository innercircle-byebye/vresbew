#7 에서 현재 상황을 표시 해봤습니다.

## 작업 목록

✓: 됨
▲: 거의 다됨
△: 아직 덜됨
x: 안됨

- 메서드 구현 △
  - 케이스별 Status Code 처리 △
  - POST 메서드 ▲ @sungyongcho
    - CGI 처리 ▲ @sungyongcho @k
    - ~~`/` 들어왔을 때 추가 조사작업 필요~~ ✓
  - DELETE 메서드 ▲ @mg5566
  - GET, HEAD 메서드 ▲ @sungyongcho
    - CGI 처리 ~~body buffer 쓰지 않고 cgi로 전달~~
	- ~~event에서 recv를 통해 받아오는 버퍼로 바로 전달(자식 프로세스 송/수신)~~ ✓
    - ~~`/` 들어왔을 때 추가 조사작업 필요~~ ✓

- 요청 메시지(Request) △ @khcho902
  - 여러 값이 들어오는 헤더에 대한 처리 방법 고민(`Accept-encoding` 등) △ @khcho902
  - `Config`의 `client_max_body_size`를 고려하여 처리하도록 구현 ▲ @khcho902
  - content-length 만큼 body 읽도록 처리하는 부분 넣기 ▲  @khcho902
  - config의 return 지시어 처리해서 status_code 세팅하기  ▲ @khcho902
   - Full URL(예시: `http://52.127.2.10/test/`)에 대한 처리 검토/구현 ▲ (한번 살펴보기)

- 응답 메시지(Response) ✓
  - 필요한 헤더들 추가 △

- `Config` ▲ @khcho902
  - "`return`, `cgi`, `cgi_path`" 디렉티브 ✓ @khcho902
  - 상속 관계 묶는 작업(리팩토링) ▲ (맨 마지막) @khcho902

- 구현
  - `RequestHandler`, `ResponseHandler` 둘다 Static 으로 선언 변경(현재는 `MessageHandler`만 Static) ✓
	- `'ResponseHandler'` 스태틱 변경이 필요한가?

- 기타 △
     - listening 열 때 throw만 해 주면 될듯
  - ~~nginx와 동일하게 하려면~~테스터 통과 필수, line by line(TELNET)으로 받을 때 오류 확인 필요함 ▲


  ## 파트 재배분
- 👆 + ...
- chunked message @mg5566
- TODO 작성 해 놓은것들 해결
    -> 위의 사항들 전부 해결 후 정리하기로...
- Logger
    -> 위의 사항들 전부 해결 후 정리하기로...
- Directory Listing (nginx autoindex) 구현 #18 @jiwlee97

- response @sungyongcho @mg5566
  - 구현이 더 필요한 동작 @sungyongcho @mg5566
  - MIME-TYPES <-  response header 만들 때 사용 @sungyongcho
- 기타 등등..

