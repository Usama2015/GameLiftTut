// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftTutGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "TextReaderComponent.h"

UGameLiftTutGameInstance::UGameLiftTutGameInstance() {
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));

	ApiUrl = TextReader->ReadFile("Urls/ApiUrl.txt");
	RegionCode = TextReader->ReadFile("Urls/RegionCode.txt");

	HttpModule = &FHttpModule::Get();
}

void UGameLiftTutGameInstance::Shutdown() {
	GetWorld()->GetTimerManager().ClearTimer(RetrieveNewTokensHandle);
	GetWorld()->GetTimerManager().ClearTimer(GetResponseTimeHandle);

	if (AccessToken.Len() > 0) {
		if (MatchmakingTicketId.Len() > 0) {
			TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
			RequestObj->SetStringField("ticketId", MatchmakingTicketId);

			FString RequestBody;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
			if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
				TSharedRef<IHttpRequest> StopMatchmakingRequest = HttpModule->CreateRequest();
				StopMatchmakingRequest->SetURL(ApiUrl + "/stopmatchmaking");
				StopMatchmakingRequest->SetVerb("POST");
				StopMatchmakingRequest->SetHeader("Content-Type", "application/json");
				StopMatchmakingRequest->SetHeader("Authorization", AccessToken);
				StopMatchmakingRequest->SetContentAsString(RequestBody);
				StopMatchmakingRequest->ProcessRequest();
			}
		}
		TSharedRef<IHttpRequest> InvalidateTokensRequest = HttpModule->CreateRequest();
		InvalidateTokensRequest->SetURL(ApiUrl + "/invalidatetokens");
		InvalidateTokensRequest->SetVerb("GET");
		//InvalidateTokensRequest->SetHeader("Content-Type", "application/json");
		InvalidateTokensRequest->SetHeader("Authorization", AccessToken);
		InvalidateTokensRequest->ProcessRequest();
	}

	Super::Shutdown();
}

void UGameLiftTutGameInstance::Init() {
	Super::Init();

	GetWorld()->GetTimerManager().SetTimer(GetResponseTimeHandle, this, &UGameLiftTutGameInstance::GetResponseTime, 1.0f, true, 1.0f);
}

void UGameLiftTutGameInstance::SetCognitoTokens(FString NewAccessToken, FString NewIdToken, FString NewRefreshToken) {
	AccessToken = NewAccessToken;
	IdToken = NewIdToken;
	RefreshToken = NewRefreshToken;

	//UE_LOG(LogTemp, Warning, TEXT("access token: %s"), *AccessToken);
	//UE_LOG(LogTemp, Warning, TEXT("refresh token: %s"), *RefreshToken);

	GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftTutGameInstance::RetrieveNewTokens, 1.0f, false, 3300.0f);
}

void UGameLiftTutGameInstance::RetrieveNewTokens() {
	if (AccessToken.Len() > 0 && RefreshToken.Len() > 0) {
		TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
		RequestObj->SetStringField("refreshToken", RefreshToken);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

		if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer)) {
			TSharedRef<IHttpRequest> RetrieveNewTokensRequest = HttpModule->CreateRequest();
			RetrieveNewTokensRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftTutGameInstance::OnRetrieveNewTokensResponseReceived);
			RetrieveNewTokensRequest->SetURL(ApiUrl + "/retrievenewtokens");
			RetrieveNewTokensRequest->SetVerb("POST");
			RetrieveNewTokensRequest->SetHeader("Content-Type", "application/json");
			RetrieveNewTokensRequest->SetHeader("Authorization", AccessToken);
			RetrieveNewTokensRequest->SetContentAsString(RequestBody);
			RetrieveNewTokensRequest->ProcessRequest();
		}
		else {
			GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftTutGameInstance::RetrieveNewTokens, 1.0f, false, 30.0f);
		}
	}
}

void UGameLiftTutGameInstance::OnRetrieveNewTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	if (bWasSuccessful) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject)) {
			if (JsonObject->HasField("accessToken") && JsonObject->HasField("idToken")) {
				SetCognitoTokens(JsonObject->GetStringField("accessToken"), JsonObject->GetStringField("idToken"), RefreshToken);
			}
		}
		else {
			GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftTutGameInstance::RetrieveNewTokens, 1.0f, false, 30.0f);
		}
	}
	else {
		GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftTutGameInstance::RetrieveNewTokens, 1.0f, false, 30.0f);
	}
}

void UGameLiftTutGameInstance::GetResponseTime() {
	TSharedRef<IHttpRequest> GetResponseTimeRequest = HttpModule->CreateRequest();
	GetResponseTimeRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftTutGameInstance::OnGetResponseTimeResponseReceived);
	GetResponseTimeRequest->SetURL("https://gamelift." + RegionCode + ".amazonaws.com");
	GetResponseTimeRequest->SetVerb("GET");
	GetResponseTimeRequest->ProcessRequest();
}

void UGameLiftTutGameInstance::OnGetResponseTimeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
	if (PlayerLatencies.Num() >= 4) {
		PlayerLatencies.RemoveNode(PlayerLatencies.GetHead());
	}

	float ResponseTime = Request->GetElapsedTime() * 1000;
	//UE_LOG(LogTemp, Warning, TEXT("response time in milliseconds: %s"), *FString::SanitizeFloat(ResponseTime));

	PlayerLatencies.AddTail(ResponseTime);
}