<?php

/**
 * Implementation of hook_rules_event_info()
 */
function fundmakr_stripe_connect_rules_event_info() {
  return array(
    'fundmakr_stripe_connect_success' => array(
      'label' => t('Stripe account connected'),
      'group' => 'Fundmakr',
      'variables' => array(
        'current_user' => array('type' => 'user', 'label' => t('The current logged in user.')),
      ),
    ),
  );
}


