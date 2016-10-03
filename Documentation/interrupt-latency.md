마이크로컨트롤러에서 인터럽트 지연은 주요 쟁점 중 하나이다. 특히나 리얼타임을 요구하는 프로그램에서 그 중대성은 더 높아진다.

인터럽트 지연은 인터럽트 요청 발생부터 실제 인터럽트 핸들러의 첫 명령이 실행되기까지 소요되는 클럭 사이클을 말한다.

많은 프로세서에서 인터럽트 지연의 정확한 측정은 인터럽트 발생시 프로세서가 실행중이던 명령 종류에 달려있다. 요컨대, 프로세서는 인터럽트 요청을 현재 실행중인 명령을 완료하고 받아들인다. 즉, 실행중인 명령어에 따라 추가적인 클럭 사이클이 소모된다. 이러한 인터럽트 지연의 불확정성이 시스템의 불안요소가 된다.

중첩된 인터럽트는 인터럽트 지연의 또다른 변수다. 우선순위가 낮은 인터럽트는 우선순위가 높은 인터럽트에 의해 실행이 중지될 수 있고 이는 막대한 인터럽트 지연 가능성을 의미한다.

이상적인 시스템에서는 다음과 같은 요구를 충족해야 한다:

* 낮은 인터럽트 지연
* 확정적인 인터럽트 응답
* 짧은 인터럽트 핸들러 수행 시간
* 상태전환(sleep)을 동반한 인터럽트 종료

인터럽트 지연은 하드웨어적 특성외에도 소프트웨어적 오버헤드가 고려되어야 한다:

* 문맥전환
* 인터럽트 서비스 루틴 종류 확인
* 인터럽트 핸들러 분기

참고 자료: [http://community.arm.com/docs/DOC-2607](http://community.arm.com/docs/DOC-2607)

인터럽트 지연을 기성 마이크로 컨트롤러와 비교분석해 Cortex-M 프로세서의 우위를 나타내는 일종의 마케팅 자료 같은데, 그와 관련 없는 일반적인 사항만 요약. (그럼에도 하나 언급할만한 것이 cortex에서 mutiple cycle instruction 실행중에 인터럽트가 발생하면 인터럽트 지연을 최소화하기 위해 실행 중이던 명령을 폐기하고 해당 인터럽트 수행이 완료된 후 폐기된 명령을 재시작. 메모리 접근일 경우엔 상태 레지스터에 현재 진행상태를 저장해두었다가 그대로 복구, 이어 실행된다고.)

파이프라인과 분기예측, 버퍼/캐시와 같은 최적화 기술 덕분에 zero jitter는 점점 실현 불가능한 이데아의 세계로.