<?php
// $Id$

/**
 * @file
 * Saves user Stripe Access Token when redirected by successful Stripe signup page.
 */

/**
* Function, process redirect.
*/

function fundmakr_stripe_connect_process_redirect() {
  if (isset($_GET['code'])) {
    $code = $_GET['code']; // authorization code from Stripe OAuth/Connect
    $client_secret = stripe_get_key('secret');
    $data = 'client_secret=' . $client_secret . '&code=' . $code . '&grant_type=authorization_code';
    $options = array(
                    'method' => 'POST',
                    'data' => $data,
            );
    $result = drupal_http_request('https://connect.stripe.com/oauth/token', $options);
    $json = json_decode($result->data);

    // Handle error response from Stripe
    if (!empty($json->error)) {
      $stripe_message = array(
        'message' => 'There was an error connecting your Stripe account.' . $json->error,
        'severity' => 'error',
      );
      watchdog('fundmakr stripe connect', $json->error_description, array(), WATCHDOG_ERROR);
    }
    else {
      // Load current user so that we can run additional security checks
      global $user;

      $edit = user_load($user->uid);
      $edit->field_stripe_connect[LANGUAGE_NONE][0]['stripe_access_token'] = $json->access_token;
      $edit->field_stripe_connect[LANGUAGE_NONE][0]['stripe_publishable_key'] = $json->stripe_publishable_key;
      $edit->field_stripe_connect[LANGUAGE_NONE][0]['stripe_user_id'] = $json->stripe_user_id;
      $stripe_account = fetch_stripe_account_data($json->access_token);
      $edit->data['user_currency'] = $stripe_account->default_currency;
      //Field attach update
      field_attach_update('user', $edit);

      $stripe_message = array(
      'message' => 'Donations are now accepted through Stripe.',
      'severity' => 'status',
      );
      if (module_exists('rules')) {
        rules_invoke_event('fundmakr_stripe_connect_success', $edit);
      }
    }
  }
  // Let's show more specific messages to user based on error type returned.
  elseif ((isset($_GET['error'])) && ($_GET['error'] == 'access_denied')) {
    $stripe_message = array(
    'message' => 'You denied access to your Stripe account.',
    'severity' => 'warning',
    );
  }
  else {
    $stripe_message = array(
    'message' => t($_GET['error_description']),
    'severity' => 'error',
    );
  }
  drupal_set_message($stripe_message['message'], $stripe_message['severity']);

  // We use the set cookie value instead of a Stripe redirect parameter since our referring pages are dynamic. See https://stripe.com/docs/connect/reference.
  drupal_goto('dashboard');
}

function fetch_stripe_account_data($user_access_token) {
  // Try to load the library and check if that worked.
  if (($library = libraries_load('stripe')) && !empty($library['loaded'])) {
    try {
      return \Stripe\Account::retrieve($user_access_token);
    }
    catch(Exception $e) {
      watchdog('stripe', 'Stripe_Account::retrieve Exception: %str', array('%str' => $e->getMessage()));
      drupal_set_message('Something went wrong. Please contact the Administrator.', 'error');

      return;
    }
  }

}
