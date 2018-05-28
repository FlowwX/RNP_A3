# RNP_A3
Building server-client network application using docker containers.

##INSTRUCTIONS
1.	pull repo
2.	run following commands from top-level folder:
	*	docker compose build 						<- builds image
	*	docker-compose up --scale ubuntu-rnp=2 -d 	<- start 2 instances
3.	ready to go for logging on to bash so run these commands:
	*	docker container ls 						<- get running instances
	*	docker exec -it {CONTAINER ID} bash  		<- pick up specific ID and log in to bash
4.	thats all - enjoy
